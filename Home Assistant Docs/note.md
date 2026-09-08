# Note
This file contains information about Home Assistant, how it is used, and how it works with our hardware.

## HA Initialization
When RPI is ready, enter this in a browser:
"http://\<RPI3_ADDRESS>:8123"
If white screen with HA logo, enter this in a browser:
"http://\<RPI3_ADDRESS>:4357"
If everything is green, then the RPI is being bottlenecked due to its limitations. Wait like 20 minutes before entering the other URL into a browser (close all HA instances so that the browsers won't take resources from the RPI).

## Adding Devices (old)
Adding matter device
open HA app -> settings -> device & services -> Search and select "Matter" -> use official matter server (box selected) -> scan QR code or hold phone near Matter device -> device should be connected after naming it

## Adding Home Assitant Docker to Raspberry Pi OS x64 (for RPI3) (CURRENT)
# Getting Raspberry Pi OS on a microSD
1. Select Device, OS, storage, create an account, and setup localisation
2. Skip Wi-Fi
3. Enable SSH
4. Write to the MicroSD

# Getting Docker (run these commands)
1. sudo apt update
2. sudo apt install ca-certificates curl
3. sudo install -m 0755 -d /etc/apt/keyrings
4. sudo curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc
5. sudo chmod a+r /etc/apt/keyrings/docker.asc
6. echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
7. sudo apt update
8. sudo apt install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
9. sudo docker run hello-world
10. You should get "Hello from Docker!" from step 9 if Docker is working.

# Getting HA (run these commands and follow instructions)
1. mkdir -p ~/homeassistant/config
2. mkdir -p ~/homeassistant/matter-server
3. cd ~/homeassistant
4. nano ~/homeassistant/compose.yaml
5. In the opened file, put the contents of compose_yaml.txt, which is found in the GitHub repo in the same directory as this Markdown file.
6. Save (ctrl + s) and exit (ctrl + x)
7. cd ~/homeassistant
8. sudo docker compose up -d
9. sudo docker compose ps
10. The previous command should show that HA is running. If there is a message about Bluetooth, ignore it.
11. In a broswer, type in "http://\<RPI3_ADDRESS>:8123". You can use "hostname -I" to find the address in the SSHed terminal, which should be the first one.

# Getting Matter Server (run these commands and follow instructions)
1. mkdir -p ~/homeassistant/matter-server
2. nano ~/homeassistant/matter-server/compose.yaml
3. In the opened file, put the contents of matter_server_compose_yaml.txt, which is found in the GitHub repo in the same directory as this Markdown file.
4. cd ~/homeassistant/matter-server
5. sudo docker compose up -d
6. sudo docker compose ps
7. The previous command should show that matterjs-server is running.
8. sudo docker compose logs -f
9. Previous command shows logs for the matter server. Do (ctrl + c) to exit logs.
10. sudo docker logs matterjs-server
11. If the previous command show that the matter server is failing due to "EACCESS: permission denied" do the following:
12. cd ~/homeassistant/matter-server
13. sudo docker compose down
14. ls -ld ~/homeassistant/matter-server/data
15. docker run --rm --entrypoint id ghcr.io/matter-js/matterjs-server:latest
16. Take note of the uid and gid.
16. sudo chown -R \<UID>:\<GID> ~/homeassistant/matter-server/data
17. If there is no ~/homeassistant/matter-server/data, make it. (mkdir -p ~/homeassistant/matter-server/data), then sudo chown -R \<UID>:\<GID> ~/homeassistant/matter-server/data
18. cd ~/homeassistant/matter-server
19. sudo docker compose up -d
20. sudo docker compose logs -f
21. sudo docker compose ps

# Adding Matter in HA
1. Open HA
2. Settings -> Devices & services -> Add integration
3. Search and click the ones that says "Matter" and wait for it to finish.
4. To be continued

# Debugging
1. No username
If there is no username, it should default to "pi".
2. SSH not responding (please read fully)
In a terminal such as Powershell, do "arp -a". The address should start as 192.168. It could also be something different.
Try to SSH into each one with "ssh \<USERNAME>@\<ADDRESS>".
The address for a Pi should respond with connection refused, while the others will have an unknown error. If multiple has connection refused, use other methods to identify the Pi.
Take out the microSD card and check if it has a file titled "ssh" with no file extension.
If there is no "ssh", correctly add one, which should enable SSH. If the address is still giving problems, use "ipconfig" in a terminal. Get the gateway address, and in a broswer type "http://\<GATEWAY>". This should list all connected devices. The one we are looking for is "PowerMesh". This will show the address for the Pi.

## Adding HA OS to a microSD (DEPRECATED)
RPI Micro SD configuration:
https://www.home-assistant.io/installation/raspberrypi/
Notes from the link:
1. Download RPI Imager and launch it
2. Select RPI3 as device
3. Select Other SPECIFIC-purpose OS (OS Section)
4. Select Home Automation and select Home Assistant (OS Section)
5. There should only be one HA option to choose from. Select it (OS Section)
6. In the storage section, select the Micro SD we are writing it to
7. Click next and it should begin writing HA OS to the Micro SD

## RPI Debugging
RPI Debugging:
1. Check if the RPI is receiving enough power. This can be checked by seeing a constant red LED on the RPI. If it is flickering, the RPI3 is not receiving the constant power it needs.
2. To see if HA is working, look if there is green flickering on the RPI. If there is green flickering, then HA is working.
3. If there is a white screen with HA logo only when doing http://homeassistant.local:8123/, use http://IP_ADDRESS:8123 or the app instead. This is likely due to browser cache issues or hardware limitations of the RPI3.

## HA Companion App
Home Assistant Companion App Download & Instructions:
https://companion.home-assistant.io/docs/getting_started/

Note: This may be for HA OS, and instructions for HA Docker may be different.

### Misc
Home Assistant Green:
https://www.home-assistant.io/green/