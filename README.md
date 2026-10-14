# Fixed RAM Location (`RAM_BLOCK = 1`)

## 1. Objective

To implement `NvM_SetRamBlockStatus()` so that when `RAM_BLOCK == 1`, the RAM mirror of a memory block is assigned to a fixed location in `NvM_MirrorTable[]`. The function must reject an invalid mirror slot or an incorrect RAM address.

## 2. Question Analysis

The question requires the RAM mirror of a particular NVM block to remain at a predefined location instead of allowing it to point to any arbitrary RAM variable.

For example, if a block is configured with `RAM_BLOCK = 1` and `mirror_idx = 3`, its RAM mirror must point to:

`NvM_MirrorTable[3]`

When `NvM_SetRamBlockStatus()` is called, the implementation must verify that the configured RAM address matches this fixed location before marking the block as changed.

### Necessary Conditions

1. **Fixed RAM location:** When `ram_block == 1`, the configured RAM address must match the selected mirror-table slot.
2. **Valid mirror index:** The mirror index must be within the available range of `NVM_MIRROR_SLOTS`.
3. **Automatic assignment:** If the RAM address is `NULL`, the implementation assigns the address of the selected mirror slot.
4. **Address validation:** If an address is already configured but differs from the required address, the function returns `NVM_ERR_ADDR_MISMATCH`.
5. **Valid block ID:** The block ID must be within the supported range and cannot be zero.
6. **Status update:** The block's validity and changed status are updated only after successful address validation.

## 3. Project Approach

The implementation uses a mirror table containing four RAM slots, each with a capacity of 64 bytes.

The temperature block is configured with:

- `TEMP_ID = 1`
- `TEMP_SLOT = 3`
- `ram_block = 1`
- `ram_addr = NULL`
- `length = sizeof(int16_t)`

The implementation follows this sequence:

1. Configure the temperature block to use mirror slot 3.
2. Initialize the mirror table and block administration data.
3. Validate the configured RAM address using `NvM_CheckRamLocation()`.
4. Automatically assign the correct address if the configured address is `NULL`.
5. Reject a different address with an error.
6. Update the block status only after successful validation.
7. Verify the behavior using a normal save operation and an intentional incorrect-address test.

### Implementation Flow

```text
NvM_SetRamBlockStatus()
          |
          v
    Validate Block ID
          |
          v
  Check RAM Block Location
          |
          v
  RAM_BLOCK == 1?
          |
          v
 Validate Mirror Slot
          |
          v
 Assign or Verify Fixed Address
          |
          v
   Address Correct?
      /        \
    Yes         No
     |           |
     v           v
 Update Status  Return Error
     |
     v
 Return NVM_OK
```

## 4. Pin Configuration

The project uses the S32K144 microcontroller and the S32 Design Studio SDK. Pin configuration is required for the ADC input, push buttons, and UART logging. The NVM mirror table itself is a software memory structure and does not require a dedicated physical pin.

| Peripheral / Signal | Configuration | Purpose |
|---|---|---|
| ADC input | `INST_ADC_CONFIG_1`, channel 0 | Reads the knob/potentiometer value used to adjust the temperature |
| PTC12 | GPIO input | SW2 save button |
| PTC13 | GPIO input | SW3 tamper-test button |
| LPUART1 TX/RX | UART peripheral pins | Sends status messages to a serial terminal |
| RAM mirror table | Software memory allocation | Stores the temperature block at its configured mirror slot |

### Pin Configuration Procedure

1. Open the project in S32 Design Studio.
2. Open the Pins Tool and inspect `pin_mux.c` and `pin_mux.h`.
3. Confirm that PTC12 and PTC13 are configured as GPIO inputs with the appropriate electrical settings.
4. Confirm that the selected ADC input is configured for the potentiometer.
5. Confirm that the LPUART1 TX/RX pins are assigned to the UART peripheral.
6. Check `sdk_project_config.h` and the generated peripheral configuration files.
7. Initialize the clock and pin configuration before using the peripherals.

**Important:** The actual pin assignments must match the board wiring and generated configuration. No additional pin is needed for `NvM_MirrorTable[3]` because it is located in MCU RAM.

## 5. Important Functions and Their Roles

### 5.1 `NvM_Init()`

**Necessity:** Initializes the software NVM management structures before reading or writing blocks.

**Procedure:**
1. Clears the RAM mirror table.
2. Resets the block administration information.
3. Prepares the software state for further operations.

**Role:** Ensures that the mirror table and block status start from a known state.

### 5.2 `NvM_CheckRamLocation()`

**Necessity:** Enforces the fixed-address requirement for blocks configured with `RAM_BLOCK = 1`.

**Procedure:**
1. Checks whether `ram_block` equals `1`.
2. Verifies that `mirror_idx` is within the available mirror slots.
3. Obtains the address of `NvM_MirrorTable[mirror_idx]`.
4. Assigns that address when `ram_addr` is `NULL`.
5. Returns `NVM_ERR_ADDR_MISMATCH` if the configured address is different.
6. For non-fixed blocks, returns `NVM_ERR_NO_RAM` if no RAM address is configured.

**Role:** This is the central validation function that guarantees the block uses the correct RAM mirror.

### 5.3 `NvM_SetRamBlockStatus()`

**Necessity:** Validates the block configuration before updating its status.

**Procedure:**
1. Checks whether the block ID is valid.
2. Calls `NvM_CheckRamLocation()` to validate the RAM address.
3. Returns immediately if validation fails.
4. Sets the block's validity flag when `changed` is `true`.
5. Updates the block's changed flag.
6. Returns `NVM_OK` when the operation succeeds.

**Role:** Connects RAM-address validation with the block-status management process.

### 5.4 `NvM_ReadAll()`

**Necessity:** Restores previously saved data into the correct RAM mirror.

**Procedure:**
1. Iterates through the configured NVM blocks.
2. Checks the block length.
3. Validates the RAM address.
4. Copies stored data into the configured RAM mirror when valid data exists.
5. Initializes the RAM contents to zero if no valid saved data exists.

**Role:** Ensures that restored data is placed in the correct memory location.

### 5.5 `NvM_WriteAll()`

**Necessity:** Saves modified RAM data to the simulated nonvolatile storage.

**Procedure:**
1. Iterates through configured blocks.
2. Skips blocks that are not marked as changed.
3. Validates the RAM address.
4. Copies the RAM data into the simulated NVM storage.
5. Updates the storage marker and clears the changed flag.

**Role:** Demonstrates how the fixed RAM mirror is used when saving data.

### 5.6 `Demo_Init()`

**Necessity:** Performs the initial setup for the complete demonstration.

**Procedure:**
1. Initializes the clock and pins.
2. Configures and calibrates the ADC.
3. Initializes LPUART1.
4. Calls `NvM_Init()` and `NvM_ReadAll()`.
5. Restores the saved temperature or stores a default value of 25°C.
6. Prints the mirror address and initial status through UART.

**Role:** Prepares the hardware and software before the main demonstration starts.

### 5.7 `Demo_Step()`

**Necessity:** Demonstrates fixed-address validation during normal operation and during an intentional error test.

**Procedure:**
1. Reads the ADC and converts the value to a temperature between 10°C and 40°C.
2. Detects a save-button press and requests a save.
3. Detects a tamper-button press and temporarily changes the block's RAM address to an incorrect buffer.
4. Calls `NvM_SetRamBlockStatus()` to verify that the incorrect address is rejected.
5. Restores the original address and prints the result through UART.

**Role:** Provides a practical demonstration of the fixed RAM location requirement.

### 5.8 `NvM_IsValid()` and `NvM_PeekNv()`

- **`NvM_IsValid()`** checks whether a block is marked as containing valid data.
- **`NvM_PeekNv()`** reads the saved data from the simulated nonvolatile storage without changing the RAM mirror.

These functions help verify whether data has been restored or saved successfully.

## 6. Implementation Procedure in S32 Design Studio

1. Import and open the S32K144 project in S32 Design Studio.
2. Inspect `nvm demo.h` for the block configuration structure, status definitions, mirror-table dimensions, and function declarations.
3. Open `nvm demo.c` and locate `NvM_MirrorTable`, `NvM_BlockCfg`, and `NvM_CheckRamLocation()`.
4. Configure the temperature block with `ram_block = 1`, `mirror_idx = 3`, and `ram_addr = NULL`.
5. Implement or verify the fixed-address checks in `NvM_CheckRamLocation()`.
6. Ensure `NvM_SetRamBlockStatus()` validates the address before changing the block status.
7. Check the ADC, button, clock, and LPUART pin/peripheral configuration.
8. Build the project and resolve any compilation errors.
9. Flash the program to the S32K144 board.
10. Open a serial terminal using the configured LPUART1 settings.
11. Test normal saving and the incorrect-address test using the buttons.

## 7. Testing and Expected Results

| Test Case | Expected Result |
|---|---|
| Valid block ID and valid mirror slot | `NvM_SetRamBlockStatus()` returns `NVM_OK` |
| RAM address is `NULL` for a fixed block | The correct mirror address is assigned |
| RAM address points to a different buffer | `NVM_ERR_ADDR_MISMATCH` is returned |
| Mirror index is outside the allowed range | `NVM_ERR_BAD_SLOT` is returned |
| Block ID is zero or outside the supported range | `NVM_ERR_BAD_ID` is returned |
| Block is marked as changed and saved | Data is copied to the simulated NVM storage |

### Expected Demonstration

- The UART terminal displays the selected mirror slot and its RAM address.
- A save operation stores the current temperature in simulated NVM storage.
- The tamper test reports an address mismatch when the block points to the wrong buffer.
- The original RAM address is restored after the tamper test.

## 8. Concepts Learned

- Fixed RAM addressing and memory mapping.
- RAM mirror tables and block configuration.
- NVM block validity and changed-status management.
- Pointer validation and error handling in C.
- Separating RAM data from simulated nonvolatile storage.
- Peripheral initialization and UART-based debugging in S32 Design Studio.

## 9. Conclusion

The exercise demonstrates how `NvM_SetRamBlockStatus()` can enforce a fixed RAM location when `RAM_BLOCK = 1`. The implementation validates the block ID, mirror index, and RAM address before updating the block status. Using `NvM_MirrorTable[3]` as the required location and deliberately testing an incorrect address helps verify that the fixed-address requirement is enforced.
