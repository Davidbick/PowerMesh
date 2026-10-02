# Matter Transportation
There are various ways to transport the Matter payload to the Matter enabled end-device. Here are some options and some possible notes on how to implement them using the resources we have.

## Option 1: Create Matter Payload at the End
1. Capture the Home Assistant (HA) command that will be turned into a JSON. This would mean capturing the Matter Server WebSocket API [GitHub Documentation.](https://github.com/matter-js/matterjs-server/blob/main/docs/websockets_api.md?utm_source=chatgpt.com) This could be accomplished by using a proxy between HA and the Matter server, since the Matter server constructs the Matter payload.
2. Parsing of the command might be needed. Some essential fields to keep are node_id, endpoint_id, cluster_id, command_name, and payload (along with its contents). Others may also be necessary. Create the JSON. According to the documentation on point 1, device_command has all of this information.
3. Use a device with Matter SDK to dynamically create a Matter payload using the JSON.
4. For the Matter payload to actually get on the end-device, use an ethernet cable or make the end-device be the device that create the Matter payload for itself.

## Option 2: Point to Point Protocol (PPP)
1. Get Home Assistant on the RPI3 using linux. HA OS would have resitrictions that would make having PPP to automatically frame the Matter payload difficult.
2. Matter is IPv6. Use PPP to frame those IPv6 packets so that it can be sent serially, like through UART. Can also be used with SPI, but SPI is naturally more difficult. PPP essentially frames the IPv6 packets that contains the Matter payload.
3. Use UART to send the framed IPv6 packets from the RPI3 to the STM32.
4. Use UART/SPI to keep transporting everything. Besides the end-device, everything should just be transport.
5. Since we framed the IPv6 packets, it would have to be deframed somewhere. This would be at the end.

## Option 3: Ethernet Cable Everything
1. Simply have all devices have ethernet ports to transport the Matter payload, since Matter commands can be transported using ethernet. Also solves problems with Matter multicast (RPI3 communicating with multiple devices) and discovery (mDNS to announce that the device is back if power goes out). Makes sure that everything can handle IPv6 and ethernet compatibility.