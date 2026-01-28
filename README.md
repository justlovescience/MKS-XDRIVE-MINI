Here you can find files for MKS XDRIVE MINI FOC driver. I found this driver after I decided to build wheeled quadruper robot, and cos I need 16 drivers for it price is really matters. I did not find proper guides or instructions for it, so I hope this repo will be helpful. I don't have any affilation with manufacturer.

Link for youtube instruction:

Instruction for windows:

Install python https://www.python.org/downloads/ select add python.exe to PATH in installer scren 
open powershell
write pip install odrive=-0.5.1.post0
wait till installed 
run by typing odrivetool 
you will see console interface where you can put commands from my_config.txt 
only thing you need to chnage is odrv0.axis0.motor.config.pole_pairs = 20, if you not sure just calculate how many magnets your motor have and divide by 2 
I use LA8308 170kV with 20 pole pairs 

Awesome WEB GUI interface: https://github.com/MoonLighTingPY/odrive3.6_web_gui

Instructions for MAC: coming soon, odrivetool is working on mac, GUI is not. 

Some useful notes: 
- Don't try to use old Odrive GUI its broken
- This driver comes with 0.5.1 firmware, upgrade by odrivetool bricks the driver, but you can unbrick it with St-link programmer, you just need dump original firmware with st-link before experiment with firmwares.
- I tried updating it to "latest" 0.5.6 but motor did not react, maybe different pins assigned to drv8301, I plan to figure this out and add it here too. 
- Driver has a "ghost" second axis cos old odrives was made for two motors. this resulting to CAN conflicts when you connect multiple drivers by CAN. Cos by default every driver have axis0 CAN_ID = 0 axis1 CAN_ID = 1. Just change axis1 CAN_ID to 63, its listen-only mode. How-to in video.
- If you want to connect it with arduino check if your board have CAN first.
- For ESP you will need to put ODriveEsp32Twai.hpp adapter in your arduino/libraries/OdriveArduino/src folder 



Shematics for esp32-S3: 
<img width="2432" height="1155" alt="esp32_s3_two_drivers_schematic" src="https://github.com/user-attachments/assets/a599c1f9-8c19-401d-9544-9ee480b77bb5" />
