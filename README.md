# G.N.O.S.I.S.-General-Operator
Touch controller and sequencer eurorack module.

**G.N.O.S.I.S. – General Operator** is a touch-controlled Eurorack sequencer and performance interface you can play by hand and / or clock signals. I’ve been wanting to design a DIY version of a touch based controller module in the Eurorack format for a while, since I couldn’t really find (m)any DIY projects going in the direction I wanted.

The project was originally inspired by [Look Mum No Computer’s 2001 keyboard sequencer](https://www.lookmumnocomputer.com/2001keyboardsequencer) concept. The circuit has been modified and extended. The firmware has been completely rewritten to implement additional functionality. I also found some inspiration in the [PacificCV Controller from maxhirez](https://www.instructables.com/PacificCV-for-Modular-Synths/) as well as this [DIY Touch Controller by HAGIWO](https://note.com/solder_state/n/ncd42925f726b).

The module has 8 touchpads that control two rows of CV outputs. Individual step outs can be switched between Trigger, Gate and Off. Inputs are: clock, direction, reset, zero and mode (wenn high, touchpads don’t trigger TGate, only pressure out) inputs. Outputs are: CVmerge (16 step combined CV output), CV1 (upper row), CV2 (lower row), Trigger Out, TGate (triggered by touchpads) and pressure Out.

This is a fully open-source project. You can find the schematics, KiCad files, BOM, firmware and gerbers on GitHub.

Copyright (C) 2026 Fred Roessler (fretze@posteo.de).

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

