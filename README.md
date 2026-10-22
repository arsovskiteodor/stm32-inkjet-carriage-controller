# STM32 Inkjet Carriage Controller

Firmware for the carriage of an inkjet printer, running on an **STM32F429I-DISC1** board. It was built in
three phases for the *Peripheral Technologies* course (La Salle, 2025–26), and each phase builds on the
previous one:

1. **Encoder tracking:** decodes a linear quadrature encoder to follow the carriage, fire a pulse for the
   printheads every 50 µm and measure the carriage speed.
2. **Pen-to-paper sensing:** samples two distance sensors with ADC and DMA, locked to the encoder position,
   to measure the gap between printhead and paper (PPS) across the page.
3. **Display:** draws the front and rear PPS profiles on the board's LCD after every print pass.

## Phase 1: Encoder tracking

The carriage moves over a 5 lines/mm encoder strip at 10–20 cm/s. The two channels, A and B, are 90° apart,
and an index signal (IDX) marks the start of the 60 cm print zone.

- **Encoder simulator:** TIM9 generates A and B in quadrature on PE5/PE6. Two pull-down GPIOs read at
  startup (PG3, PG9) choose one of four speeds: **10, 13, 16 or 20 cm/s**. Wiring these outputs back to the
  inputs lets the board test itself.
- **Decoding:** both edges of A and B trigger EXTI interrupts (PD4, PD5). Each ISR works out the direction
  from the edge and the level of the other channel, which gives 4 edges per line, or 50 µm per edge.
- **Print pulse:** every edge restarts TIM3 in one-pulse mode, which outputs a 50 µs `OUTPULSE` on PC8
  with no CPU time spent on the pulse itself.
- **End of print:** after 12,000 edges (60 cm), `OUTEOP` (PE2) is raised. The average speed is computed
  from the 32-bit TIM2 time base, which ticks at 1 MHz.
- **Error detection:** if the carriage reverses inside the print zone, the pass is aborted and `OUTERR`
  (PC11) and the red LED are raised.

## Phase 2: PPS sensing with ADC and DMA

Two analog sensors measure the pen-to-paper distance (0–2 mm) at the front and rear of the carriage.

- ADC1 and ADC2 are triggered from the encoder ISR on **every 4th edge (every 200 µm)**, so each sample is
  tied to a position on the page rather than to a time.
- Two DMA2 streams move the results straight into dedicated RAM buffers, **3,000 samples per sensor**
  per 60 cm pass. The CPU never copies the samples.
- After the pass, each buffer is converted to millimetres. It is reduced to **300 points (one every 2 mm)**
  using a 5-sample moving average, and the min, max and average are computed.

## Phase 3: LCD visualization

- The 320×240 LCD is driven by the LTDC with the frame buffer in external SDRAM (FMC). Two
  **ARGB4444** layers are blended, so a static background with the window frames sits under a data layer.
- After each pass, the front and rear PPS profiles are plotted in two separate windows.
- **Optimized erasing:** each window remembers the band of rows its previous plot used, and only that
  band is cleared instead of the whole window. The erase time is measured with TIM2.

## Hardware

| Peripheral | Use |
|------------|-----|
| TIM9 (CH1/CH2, PE5/PE6) | Encoder A/B simulator |
| EXTI4 / EXTI9_5 (PD4, PD5) | Encoder A/B inputs, both edges |
| EXTI0 (PA0, user button) | IDX: start of the print zone |
| TIM3 CH3 (PC8) | 50 µs one-shot `OUTPULSE` |
| TIM2 | 32-bit, 1 MHz free-running time base |
| ADC1 / ADC2 + DMA2 | Front and rear PPS sensors |
| LTDC + FMC SDRAM + SPI5 | On-board LCD |
| PE2 / PC11, LEDs PG13 / PG14 | `OUTEOP` / `OUTERR` and status LEDs |
| PG3 / PG9 | Speed selection (read at startup) |
| PG2 | Debug pin for measuring ISR timing on an oscilloscope |

The system clock is 180 MHz.

## Building

1. Open **STM32CubeIDE** and choose *File → Import → Existing Projects into Workspace*, then select this folder.
2. Build and flash it to an STM32F429I-DISC1 over the on-board ST-LINK.

To build from the command line instead (Windows, with paths adjusted to your install):

```
stm32cubeidec.exe --launcher.suppressErrors -nosplash ^
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild ^
  -data C:\path\to\temp-workspace ^
  -import C:\path\to\stm32-inkjet-carriage-controller ^
  -build stm32-inkjet-carriage-controller/Debug
```

`stm32-inkjet-carriage-controller.ioc` holds the full STM32CubeMX configuration. The HAL, CMSIS and the board's LCD/SDRAM BSP drivers
are included under `Drivers/`.

## Authors

Teodor Arsovski and Pau Arasa
