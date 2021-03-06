/*******************************************************************************
*   (c) 2018 Totient Labs
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "os.h"
#include "vetDisplay.h"

static const uint8_t const BASE_GAS_PRICE[] = {0x03, 0x8D, 0x7E, 0xA4, 0xC6, 0x80, 0x00};
static const uint8_t const MAX_GAS_COEF[] = {0xFF};
static const uint8_t const TICKER_VTHO[] = "VTHO ";

uint32_t getStringLength(uint8_t *string) {
    uint32_t i = 0;
    while (string[i]) {
        i++;
    }
    return i;
}

void convertUint256BE(uint8_t *data, uint32_t length, uint256_t *target) {
    uint8_t tmp[32];
    os_memset(tmp, 0, 32);
    os_memmove(tmp + 32 - length, data, length);
    readu256BE(tmp, target);
}

void addressToDisplayString(uint8_t *address, cx_sha3_t *sha3Context, uint8_t *displayString) {
    displayString[0] = '0';
    displayString[1] = 'x';
    getVetAddressStringFromBinary(address, displayString + 2, sha3Context);
}

void sendAmountToDisplayString(txInt256_t *sendAmount, uint8_t *ticker, uint8_t decimals, uint8_t *displayString) {
    uint256_t sendAmount256;
    convertUint256BE(sendAmount->value, sendAmount->length, &sendAmount256);
    amountToDisplayString(&sendAmount256, ticker, decimals, displayString);
}

void maxFeeToDisplayString(txInt256_t *gaspricecoef, txInt256_t *gas, feeComputationContext_t *feeComputationContext, uint8_t *displayString) {
    convertUint256BE(MAX_GAS_COEF, sizeof(MAX_GAS_COEF), &feeComputationContext->maxGasCoef);
    convertUint256BE(BASE_GAS_PRICE, sizeof(BASE_GAS_PRICE), &feeComputationContext->baseGasPrice);
    convertUint256BE(gaspricecoef->value, gaspricecoef->length, &feeComputationContext->gasPriceCoef);
    convertUint256BE(gas->value, gas->length, &feeComputationContext->gas);
    // (BGP * GPC)
    mul256(&feeComputationContext->gasPriceCoef, &feeComputationContext->baseGasPrice, &feeComputationContext->tmp);

    // (BGP / 255) * GPC
    divmod256(&feeComputationContext->tmp, &feeComputationContext->maxGasCoef, &feeComputationContext->maxFeeBonus, &feeComputationContext->gasPriceCoef);
    // (1 + BGP / 255) * GPC
    add256(&feeComputationContext->maxFeeBonus, &feeComputationContext->baseGasPrice, &feeComputationContext->tmp);
    // (1 + BGP / 255) * GPC) * G
    mul256(&feeComputationContext->tmp, &feeComputationContext->gas, &feeComputationContext->maxFee);

    amountToDisplayString(&feeComputationContext->maxFee, TICKER_VTHO, DECIMALS_VTHO, displayString);
}

void amountToDisplayString(uint256_t *amount256, uint8_t *ticker, uint8_t decimals, uint8_t *displayString) {
    uint8_t decimalAmount[100];
    uint8_t adjustedAmount[100];
    tostring256(amount256, 10, (char *)decimalAmount, 100);
    adjustDecimals((char *)decimalAmount, getStringLength(decimalAmount),
                   (char *)adjustedAmount, 100, decimals);
    
    uint32_t tickerLength = getStringLength(ticker);
    uint32_t adjustedAmountLength = getStringLength(adjustedAmount);
    os_memmove(displayString, ticker, tickerLength);
    os_memmove(displayString + tickerLength, adjustedAmount, adjustedAmountLength);
    displayString[tickerLength + adjustedAmountLength] = '\0';
}