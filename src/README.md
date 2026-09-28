# G.N.O.S.I.S.-General-Operator

Touch controller and sequencer eurorack module.

This repository contains the KiCad design files and Gerber production files for a custom touch-controlled Eurorack sequencer and performance interface.

The system is powered by an Arduino Nano, using an MPR121 capacitive touch controller and an MCP4725 DAC for pressure-sensitive CV output.

---

## ⚠️ Revision 2 Notice & Status

**Current Status:** Untested (Revision 2)

The repository has been updated with **Revision 2 (Rev 2)** files. This version incorporates minor fixes for PCB layout and labeling errors identified during the build and testing of Revision 1 (Rev 1).

### Rev 2 Changes & Fixes:
- **Potentiometer Wiring:** Corrected the inverted pin mapping on Pin 1 and Pin 3 so potentiometers behave as expected.
- **Schematic & Silkscreen Labels:** Corrected swapped component labels (R37/R38 and R45/R46) and added the missing label for R32 (1k).
- **Silkscreen Cleanup:** Fixed cosmetic issued concerning labeling.

> **Disclaimer:** While these changes are small functional and cosmetic corrections based on a fully working Rev 1 build, **this specific Rev 2 PCB has not been manufactured or tested by me yet**. Producing this revision right now is done at your own risk.

---

## 🔄 Future Updates

As soon as I manufacture, assemble, and verify the physical Rev 2 boards in my setup, I will test all functions and update this README with a confirmed status.

---
