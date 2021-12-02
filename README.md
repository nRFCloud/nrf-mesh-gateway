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
- Mesh model message subscription.
- Mesh model message sending.

## Process
### Configuration
The process for configuring Bluetooth mesh devices so that they can participate in a mesh network is as follows:

- Provision a device which is actively sending an unprovisioned device beacon.
- Add or generate an application key on the gateway if none currently exist.
- Bind the application key to the desired model on the node.
- Set a publish context for the model.
- Add subscription addresses to the model.

The node's model will now publish data within the parameters of the publish context you set.
The node's model will now receive mesh messages which are published to your chosen subscription addresses.
The gateway can be used to interface with node's model with the following process:

- Subscribe the gateway to mesh address which you'd like to receive.
- Receive mesh model messages.
- Send mesh model messages to the remote node's model.

#### Example - Add a Mesh light and a Mesh light switch to the network, configure them, and interface with them via the gateway.

Note: The following commands and others are defined in detail in cli_cmd_def.md.
This process can also be followed with a remote gateway via nRF Cloud by using the JSON messages defined in json_msg_def.md.

1. Ensure that the new device is broadcasting an unprovisioned device beacon (values for UUID will be different for you):
       
`beacon list`

You should see the UUID of the new device listed.

Example:
```
     $uart:~$ beacon list
     UUID: 4df5fa66d7eb44b98000000000000000
     - OOB Info: None
     - URI Hash: 0x00000000
   
     UUID: 6a4c2749bfa84db28000000000000000
     - OOB Info: None
     - URI Hash: 0x00000000
```
       
2. Provision the new device (use the `tab` button to autofill the device UUID):

`provision <device UUID>`
       
Example:
```
     $uart:~$ provision 4df5fa66d7eb44b98000000000000000
     $uart:~$ provision 6a4c2749bfa84db28000000000000000
```
       
3. Ensure that the device is now a node:

`node list`
       
You should see the newly provisioned node listed. *Note the address.

Example:
```
    $uart:~$ node list
    Network Nodes:
      UUID: 000102030405060708090a0b0c0d0e0f
      - Address      : 0x0001
      - Net Index    : 0x0000
      - Element Count: 1
      - Configured   : YES
  
      UUID: 4df5fa66d7eb44b98000000000000000
      - Address      : 0x0002
      - Net Index    : 0x0000
      - Element Count: 4
      - Configured   : NO
  
      UUID: 6a4c2749bfa84db28000000000000000
      - Address      : 0x0006
      - Net Index    : 0x0000
      - Element Count: 4
      - Configured   : NO
```
       
4. Generate a new application key under the primary network index:

`appkey generate 0x0000`

You should see the generated application details listed. *Note the application index.

Example:
```
     uart:~$ appkey generate 0x0000
     Successfully generated application key
     Application Key: 342e4bf0772a34cd46e820c0bdfb61a5
     Application Index: 0x0000
     Network Index: 0x0000
```
       
5. Bind the newly generated application key to a model of your choosing.

In this example, the following arguments are used:

For the generic on/off client (light switch):
- Node Address     : 0x0002
- Application Index: 0x0000
- Element Address  : 0x0002
- Model ID         : 0x1001 

For the generic on/off server (light bulb):
- Node Address     : 0x0006
- Application Index: 0x0000
- Element Address  : 0x0006
- Model ID         : 0x1000 

`model appkey bind <Node Address> <Application Index> <Element Address> <Model ID>`

Example:
```
     uart:~$ model appkey bind 2 0 2 0x1001
     addr     : 2
     appIdx   : 0
     elemAddr : 2
     modelId  : 0x1001
     
     Successfully binded application key to model.
     
     uart:~$ model appkey bind 6 0 6 0x1000
     addr     : 6
     appIdx   : 0
     elemAddr : 6
     modelId  : 0x1000
     
     Successfully binded application key to model.
```

6. Add a subscribe address to the model so that it can receive messages.

In this example, the following arguments are used:

For the generic on/off client (light switch):
- Node Address     : 0x0002
- Element Address  : 0x0002
- Model ID         : 0x1001
- Subscribe Address: 0xD000

For the generic on/off server (light bulb):
- Node Address     : 0x0006
- Element Address  : 0x0006
- Model ID         : 0x1000
- Subscribe Address: 0xC000

`model subscribe add <Node Address> <Element Address> <Model ID> <Subscribe Address>`

Example:
```
     uart:~$ model subscribe add 2 2 0x1001 0xd000
     addr     : 2
     elemAddr : 2
     modelId  : 0x1001
     subAddr  : 0xd000
     
     Successfully added subscribe address to model.
     
     uart:~$ model subscribe add 6 6 0x1000 0xc000
     addr     : 6
     elemAddr : 6
     modelId  : 0x1000
     subAddr  : 0xc000
     
     Successfully added subscribe address to model.
```

7. Set the publish parameter of the model so that it can send messages.

In this example, note that each node publishes to the group address the other node subscribes to.

For the generic on/off client (light switch):
- Node Address             : 0x0002
- Element Address          : 0x0002
- Model ID                 : 0x1001
- Publish Address          : 0xD000
- Publish Application Index: 0x0000
- Friend Credential Flag   : false
- Time-to-Live             : 7
- Period Key               : 100ms
- Period                   : 0
- TX Count                 : 0
- TX Interval              : 0
 
For the generic on/off server (light bulb):
- Node Address             : 0x0006
- Element Address          : 0x0006
- Model ID                 : 0x1000
- Publish Address          : 0xC000
- Publish Application Index: 0x0000
- Friend Credential Flag   : false
- Time-to-Live             : 7
- Period Key               : 100ms
- Period                   : 0
- TX Count                 : 0
- TX Interval              : 0

`model publish set <Node Address> <Element Address> <Model ID> <Publish Address> <Publish Application Index> <Friend Flag> <TTL> <Period Key> <Period> <TX Count> <TX Interval>`
    
Example:
```
     uart:~$ model publish set 2 2 0x1001 0xc000 0 false 7 100ms 0 0 0
     addr     : 2
     elemAddr : 2
     modelId  : 0x1001
     pubAddr  : 0xc000
     pubAppIdx: 0
     credFlag : false
     ttl      : 7
     periodKey: 100ms
     period   : 0
     count    : 0
     interval : 0
     
     Successfully set model publish parameters.

     uart:~$ model publish set 6 6 0x1000 0xd000 0 false 7 100ms 0 0 0
     addr     : 6
     elemAddr : 6
     modelId  : 0x1000
     pubAddr  : 0xd000
     pubAppIdx: 0
     credFlag : false
     ttl      : 7
     periodKey: 100ms
     period   : 0
     count    : 0
     interval : 0
     
     Successfully set model publish parameters.
```

Now, other models that are subscribed to the group addresses 0xC000 or 0xD000 will receive all messages published by these models.
Likewise, any messages published to the group address 0xC000 or 0xD000 by other models will be received by these models.

In order to receive mesh model messages published by the newly configured node:

8. Subscribe the gateway to same publish addresses used in step 7 to receive mesh model messages from the node
addresses: 0xC000, 0xD000

```
message subscribe 0xC000
message subscribe 0xD000
```
       
Now the gateway will receive and present messages published to the group addresses 0xC000 and 0xD000.

To send mesh model messages to the node using the gateway:

10. Send Generic OnOff Set Message:
- Net Index        : 0x0000
- Application Index: 0x0000
- Address          : 0xC000
- Opcode           : 0x8202
- Payload          : 0100

`message send 0x0000 0x0000 0xC000 0x8202 0100`
       
This will turn light 1 on.
Notice that the gateway will receive a Generic OnOff Status message and a confirmation message.

Example:
```
  [00:01:47.281,127] <dbg> app_btmesh.btmesh_msg_cb: mesh message callback
  [00:01:47.281,127] <dbg> app_btmesh.btmesh_msg_cb:     opcode : 0x00008202
  [00:01:47.281,158] <dbg> app_btmesh.btmesh_msg_cb:     net_idx: 0x0000
  [00:01:47.281,158] <dbg> app_btmesh.btmesh_msg_cb:     app_idx: 0x0000
  [00:01:47.281,158] <dbg> app_btmesh.btmesh_msg_cb:     Source Address: 0x0001
  [00:01:47.281,158] <dbg> app_btmesh.btmesh_msg_cb:     Destination Address: 0xc000
  [00:01:47.281,188] <dbg> app_btmesh: Payload
                                       01 00                                            |..
  [00:01:47.328,643] <dbg> app_btmesh.btmesh_msg_cb: mesh message callback
  [00:01:47.328,643] <dbg> app_btmesh.btmesh_msg_cb:     opcode : 0x00008204
  [00:01:47.328,674] <dbg> app_btmesh.btmesh_msg_cb:     net_idx: 0x0000
  [00:01:47.328,704] <dbg> app_btmesh.btmesh_msg_cb:     app_idx: 0x0000
  [00:01:47.328,765] <dbg> app_btmesh.btmesh_msg_cb:     Source Address: 0x0006
  [00:01:47.328,765] <dbg> app_btmesh.btmesh_msg_cb:     Destination Address: 0x0001
  [00:01:47.328,765] <dbg> app_btmesh: Payload
                                       01                                               |.
  [00:01:47.559,661] <dbg> app_btmesh.btmesh_msg_cb: mesh message callback
  [00:01:47.559,692] <dbg> app_btmesh.btmesh_msg_cb:     opcode : 0x00008204
  [00:01:47.559,692] <dbg> app_btmesh.btmesh_msg_cb:     net_idx: 0x0000
  [00:01:47.559,722] <dbg> app_btmesh.btmesh_msg_cb:     app_idx: 0x0000
  [00:01:47.559,722] <dbg> app_btmesh.btmesh_msg_cb:     Source Address: 0x0006
  [00:01:47.559,722] <dbg> app_btmesh.btmesh_msg_cb:     Destination Address: 0xd000
  [00:01:47.559,722] <dbg> app_btmesh: Payload
                                       01
```
