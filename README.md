# PowerMesh Smart Home

Trello Link: https://trello.com/b/KgnVWOYr/powermesh



For the STM32 development the workflow I'm familiar with is through vscode but if you want to use a different IDE that's okay, I just don't know what's going on with those.
The stuff you need is:

-STM32MX: This is the code generator with a GUI interface that is used for the ST MCUs. You should be able to choose our stm32f4 MCU and then it'll generate the code based on the peripherals you selected in the .ioc file. This should be a standalone download and make sure to use cmake as the build system at the last page before "generating project". You should be able to generate your .elf file this way.

OpenOCD: This is the server that you'll instantiate on your PC to communicate with the debugger on the nucelo board. There should be installation settings on their website. In Vscode you should be able to use the config files in the.vscode folder to run the openocd server and connect the debugger.

If you're debugging in VScode I recommend using the cortex-debug extension to set all this up. I've done this on linux a couple times but never on windows so I might be useless there. You should be able to connect to any IDE with Cmake and it gets rid of all that stmcubeide abstraction so you understand what's going on better.