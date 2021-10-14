# Nordic Mesh Gateway

This project acts as a Bluetooth Mesh - LTE gateway. This project also has UART shell interface that supports Bluetooth Mesh operations.

## Requirements:
- Review the project's west.yml file for NCS and Zephyr versions
- nRF 9160DK PCA10090
- onboard nRF52840 (board controller) programmed with ncs/nrf/samples/bluetooth/hci_lpuart project

## Support
Currently LTE support includes:

- Unprovisioned device beacon interfaceing.
- Provisioning of devices.
- Retrieving a list of network nodes.
- Node configuration interfacing.
- Mesh model message subscribption.
- Mesh model message sending.

Currently UART Shell support includes:

- Unprovisioned device beacon interfacing.
- Provisioning of devices.
- Retrieving a list of network nodes.
- Node configuration interfacing.
- Mesh model message subscribption.
- Mesh model message sending.

## Process
### Configuration
The process for configuring Bluetooth mesh devices so that they can participate in a mesh network is as follows:

- Provision a device which is actively sending an unprovisioned device beacon.
- Add or generate an application key on the gateway if none currently exist.
- Bind the application key to the desired model on the node.
- Set a publish context for the model.
- Add subscription addresses to the model.

The node's model will now publish data within the parameters of the publish context you set. The node's model will now recieve mesh messages which are published to you chosen subscribe addresses. The gateway can be used to interface with node's model with the followin process:

- Subscribe the gateway to mesh address for which you'd like to recieve.
- Recieve mesh model messages.
- Send mesh model messages to the remote node's model.

#### Example - Add a new device to the network, configure it, and interface with it via the gateway.
Note: The following commands and others are defined in detail in cli_cmd_def.md. This process can also be followed with a remote gateway via nRF Cloud by using the JSON messages defined json_msg_def.md.
1. Ensure that the new device is broadcasting an unprovisioned device beacon:
	
	`beacon list`
	
	You should see the UUID of the new device listed.
	
2. Provision the new device (use the `tab` button to autofill the device UUID):

	`provision <device UUID>`
	
3. Ensure that the device is now a node:

	`node list`
	
	You should see the newly provisioned node listed. *Note the address.
	
4. Generate a new application key under the primary network index:

	`appkey generate 0x0000`
	
	You should see the generated application details listed. *Note the applicaiton index.
	
5. Bind the newly generated application key to a model of your choosing. In this example, the following arguments are used:
	- Node Address     : 0x0002
	- Application Index: 0x0000
	- Element Address  : 0x0002
	- Model ID         : 0x1000 

	`model appkey bind 0x0002 0x0000 0x0002 0x1000`

6. Add a subcribe address to the model so that it can recieve messages. In this example, the following arguments are used:
	- Node Address     : 0x0002
	- Element Address  : 0x0002
	- Model ID         : 0x1000
	- Subscribe Address: 0xC000

	`model subscribe add 0x0002 0x0002 0x1000 0xC000`	
7. Set the publish parameter of the model so that it can send messages. In this example, the following arguments are used:
	- Node Address             : 0x0002
	- Element Address          : 0x0002
	- Model ID                 : 0x1000
	- Publish Address          : 0xC000
	- Publish Application Index: 0x0000
	- Friend Credential Flag   : false
	- Time-to-Live             : 7
	- Period Key               : 100ms
	- Period                   : 0
	- TX Count                 : 0
	- TX Interval              : 0

	`model publish set 0x0002 0x0002 0x1000 0xC000 0x0000 false 7 100ms 0 0 0`
	
Now, other models that are subscribed to the group address 0xC000 will recieve all messages published by this model. Likewise, any messages published to the group address 0xC000 by other models will be recieved by this model.

In order to recieve mesh model messages published by the newly configured node:

8. Subscribe the gateway to same publish address used in step 7 to recieve mesh model messages from the node:
	- Address: 0xC000

	`message subscribe 0xC000`
	
9. Subscribe the gateway to its own unicast address:
	- Address: 0x0001

	`message subscribe 0x0001`
	
Now the gateway will recieve and present messages published to the group address 0xC000 and messages published published directly to the gateway.

To send mesh model messages to the node using the gateway:

10. Send Generic OnOff Set Message:
	- Net Indes.       : 0x0000
	- Application Index: 0x0000
	- Address          : 0xC000
	- Opcode           : 0x8202
	- Payload          : 01

	`message send 0x0000 0x0000 0xC000 0x8202 01`
	
Notice that the gateway will recieve a Generic OnOff Status message twice: once with destination address 0xC000 and again with destination address 0x0001. This is because the Generic OnOff Set message is an acknowledged message where the acknowledgement is sent directly to the riginal sender, in this case, the gateway itself. Inorder to recieve only one Generic OnOff Status message, the gateway can either unsubscribe from one of the subscribed address: 0xC000 or 0x0001, or send a Generic OnOff Set Unacnkowledged message with opcode 0x8203.
