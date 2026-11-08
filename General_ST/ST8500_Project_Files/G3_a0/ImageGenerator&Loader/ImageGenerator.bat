@ECHO OFF
ImageGenerator.exe -c=config_ext.xml -b -o=ST8500_G3_EXT.img 
ImageGenerator.exe -c=config.xml -b -o=ST8500_G3.img
@ECHO ON