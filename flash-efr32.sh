arm-none-eabi-objcopy -O ihex output/efr32/bin/ot-cli-ftd output/efr32/bin/ot-cli-ftd.hex && JLinkExe -device EFR32MG1PxxxF256 -if JTAG -speed 4000 -jtagconf -1,-1 -autoconnect 1 -commanderscript flash-efr32.jlink


