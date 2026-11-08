@echo off
::Sets localization of variables.
setlocal enableExtensions disableDelayedExpansion
set DEFAULT_COMM=COM0

::Sets the default images and gets arguments
set PE_image_dl="imgs\ST8500_G3_v17704591_6.2.18.img"
set RTE_image_dl="imgs\STARCOM_G3_RT_FW_r1_15_rc2_BA_key0.img"

echo #######################################################################################
echo Before proceeding with FW upload on EVLKST8500GHxxx, 
echo please ensure that STM32 MODE switch position is "1" (DOWN) and PLC BOOT1 is "1" (DOWN)
echo Before proceeding with FW upload on EVLKST8500GH-2, 
echo please ensure that SW201 switch position is CP2105 and PLC BOOT1 is "1" (DOWN)
echo #######################################################################################
echo.
echo #################################
echo Press RESET on the evaluation kit
echo #################################
echo.
pause
echo.

cd st8500_image_loader
python image_loader_st8500.py --port %DEFAULT_COMM% --baudrate 921600 --write_to_RAM --pe %PE_image_dl% --rte %RTE_image_dl%

echo.
pause

