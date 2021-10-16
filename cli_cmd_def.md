# CLI Shell Commands
In many cases arguments can be auto-completed using the `tab` key. These are cases where the only possible options are all known by the gateway. In these cases, only the syntax provided by the auto-complete are accebted by the shell. In any other case, acceptible argument syntax is described as:

- All indexes, addresses, and ID arguments are 16-bit unsigned values. These can be entered in decimal or hexadecimal character strings. `0x` must imediately precede all hexadecimal values of these types.
- All keys are 128-bit unsigned hexadecimal values made up of 32 character characters. No preceding `0x` is required.
- All state values are boolean and can be entered using any of the following strings:
	- `0`
	- `1`
	- `disable`
	- `enable`
	- `false`
	- `true`

## Unprovisioned Device Beacon Commands
The `beacon` command set is used for interfacing with unprovisioned device beacons that are received by the gateway.

- `beacon list`

	List all active unprovisioned device beacons.

- `beacon block <UUID>`

	- `UUID` - Mandatory. 128-bit hexadecimal UUID of the device to be blocked.

	Block an unprovisioned device beacon from appearing in the unprovisioned device beacon list.

- `beacon unblock <UUID>`

	- `UUID` - Mandatory. 128-bit hexadecimal UUID of the device to be un-blocked.
	
	Un-block an unprovisioned device beacon from appearing in the unprovisioned device beacon list.
	
## Device Provisioning Command
The `provision` command is used to provision a device making it a node in the mesh network. An unprovisioned device beacon for the desired device must be received by the gateway before provisioning can take place.

- `provision <UUID> [-i netIdx] [-a addr] [-t attn]`
	- `UUID` - Mandatory. 128-bit hexadecimal UUID of the device to be provisioned.
	- `netIdx` - Optional. The network index for which the device is to be provisioned with. If no `netIdx` argument is provided, the primary network index 0x0000 will be used.
	- `addr` - Optional. The address to assign to the device. If no `addr` argument is provided, the lowest available node address will be used.
	- `attn` - Optional. The attention time duration for the device during the provisioning process. If no `attn` argument is provided, an attention time of 0 will be used.

	Provision an unprovisioned device and make it an active node in the network.
	
## Gateway Subnet Commands
The `subnet` command set is used for interfacing with the non-volatile storage of subnets on the gateway. Subnets must be added to the gateway before they can added to nodes of the mesh network.

- `subnet list`

	List all network index:key pairs for subnets that exist on the gateway.
	
- `subnet view <netIdx>`
	- `netIdx` - Mandatory. The network index of the network key to be viewed.

	View a network key for a subnet that exists on the gateway.
	
- `subnet add <netKey> [-i netIdx]`
	- `netKey` - Mandatory. 128-bit hexadecimal network key.
	- `netIdx` - Optional. Network index to be assigned to the network key. If no `netIdx` argument is provided, the lowest available network index will be used.

	Add a subnet with network key to the gateway.
	
- `subnet generate [-i netIdx]`
	- `netIdx` - Optional. Network index to be assigned to the generated network key. If no `netIdx` argument is provided, the lowest available network index will be used.

	Have the gateway generate a new network key.
	
- `subnet remove <netIdx>`
	- `netIdx` - Mandatory. Network index of the network key to be removed from the gateway.

	Remove a subnet with network key from the gateway.
	
## Gateway Application Key Commands
The `appkey` command set is used for interfacing with the non-volatile storage of application keys on the gateway. Application keys must be added to the gateway before thay can be added to nodes of the mesh network.

- `appkey list`

	List all application index:key pairs that exist on the gateway.
	
- `appkey view <appIdx>`
	- `appIdx` - Mandatory. The application index of the application key to be viewed.

	View an application key that exists on the gateway.
	
- `appkey add <netIdx> <appKey> [-i appIdx]`
	- `netIdx` - Mandatory. The subnet index that the application key will belong to.
	- `appkey` - Mandatory. 128-bit hexadecimal application key.
	- `appIdx` - Optional. Application index to be assigned to the application key. If no `appIdx` argument is provided, the lowest available application index will be used.

	Add an application key to the gateway.
	
- `appkey generate <netIdx> [-i appIdx]`
	- `netIdx` - Mandatory. The subnet index that the generaeted application key will belong to.
	- `appIdx` - Optional. Application index to be assigned to the generated application key. If no `appIdx` argument is provided, the lowest available application index will be used.

	Have the gateway generate a new application key.
	
- `appkey remove <appIdx>`
	- `appIdx` - Mandatory. Application index of the application key to be removed from the gateway.

	Remove an application key from the gateway.
	
## Node Configuration Commands
The `node` command set is used for configuring nodes on the mesh network so that they can participate in the network.

### General
General node configuration commands.
- `node list`

	List all nodes that are part of the mesh network along with their high-level details.
	
- `node discover <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get all available configuration details from a node including composition data, states, subnets, elements, models, application indexes, publish addresses, and subscribe addresses.
	
- `node comp <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get the composition data from a node which includes its identifies, elements, and model IDs.
	
### Node Features and States
Commands for configuring the states and features of nodes on the mesh network.

- `node beacon get <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get the network beacon state of a node.
	
- `node beacon set <addr> <state>`
	- `addr` - Mandatory. Address of the target node.
	- `state` - Mandatory. The enable/disable state of the node's network beacon.

	Set the network beacon state of a node.
	
- `node ttl get <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get the time-to-live value of a node.
	
- `node ttl set <addr> <ttl>`
	- `addr` - Mandatory. Address of the target node.
	- `ttl` - Mandatory. Time-to-live value of the node.

	Set a time-to-live value of a node.
	
- `node friend get <addr>`
	- `addr` - Mandatory. Address of the target node.
	
	Get the state of a node's friend feature.
	
- `node friend set <addr> <state>`
	- `addr` - Mandatory. Address of the target node.
	- `state` - Mandatory. The enable/disbale state of the node's friend feature.

	Set the state of a node's friend feature.
	
- `node proxy get <addr>`
	- `addr` - Mandatory. Address of the target node.
	
	Get the state of a node's proxy feature.
	
- `node proxy set <addr> <state>`
	- `addr` - Mandatory. Address of the target node.
	- `state` - Mandatory. The enable/disable state of the node's proxy feature.

	Set the state of a node's proxy feature.
	
- `node relay get <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get the state, count, and interval of a node's relay feature.
	
- `node relay set <addr> <state> <count> <interval>`
	- `addr` - Mandatory. Address of the target node.
	- `state` - Mandatory. The enable/disbale state of the node's relay feature.
	- `count` - Mandatory. Number of retransmissions (first transmission excluded) of the node's relay feature. Values within range 0-7 are valid.
	- `interval` - Mandatory. Millisecond interval of retransmissions for the node's relay feature. Values within range 1-320 and are multiples of 10 are valid.

	Set the state, count, and interval of a node's relay feature.

### Node Heartbeat
Commands for setting the subscribe and publish parameters of a node's heartbeat.

- `node hb-sub-set <addr> <src> <dest> <period> <min> <max>`
	- `addr` - Mandatory. Address of the target node.
	- `src` - Mandatory. The source address of heartbeat messages to subscribe to.
	- `dest` - Mandatory. The destination address of heartbeat messages to subscribe to.
	- `period` - Mandatory. Logarithmic subscription period to listen for heartbeat messages. (1 << (count - 1)).
	- `count` - Mandatory. Logarithmic heartbeat message receive count. (1 << (count - 1)).
	- `min` - Mandatory. Minimum number of hops for received heartbeat messages.
	- `max` - Mandatory. Maximum number of hops for received heartbeat messages.

	Set a node's heartbeat subscribe parameters.

- `node hb-pub-set <netIdx> <addr> <dest> <count> <period> <ttl> <relay> <proxy> <friend> <lpn>`
	- `netIdx` - Mandatory. The net index to encrypt heartbeat messages with.
	- `addr` - Mandatory. Address of the target node.
	- `dest` - Mandatory. Destination of published heartbeat messages.
	- `count` - Mandatory. Logarithmic heartbeat message count. (1 << (count - 1)).
	- `period` - Mandatory. Logarithmic hearthbeat message publish period. (1 << (count - 1)).
	- `ttl` - Mandatory. Time-to-live for published heartbeat messages.
	- `relay` - Mandatory. Enable/disable relay feature trigger of heartbeat messages.
	- `proxy` - Mandatory. Enable/disable proxy feature trigger of heartbeat messages.
	- `friend` - Mandatory. Enable/disable friend feature trigger of heartbeat messages.
	- `lpn` - Mandatory. Enable/disable low-power-node feature trigger of heartbeat messages.

	Set a node's heartbeat publish parameters.

### Node Subnets
Commands for configuring subnet keys of a node on the mesh network.

- `node subnet add <addr> <netIdx>`
	- `addr` - Mandatory. Address of the target node.
	- `netIdx` - Mandatory. The network index of the subnet to be added to the node.

	Add a subnet along with its index and key to a node.
	
- `node subnet get <addr>`
	- `addr` - Mandatory. Address of the target node.

	Get a list of network indexes for the subnets that a node is a part of.
	
## Model Configuration Commands
The `model` command set is used for configuring models that exist on nodes within the mesh network.

### Model Application Key Commands
Commands for configuring application keys used by Bluetooth mesh models within elements on nodes.

- `model appkey bind <addr> <appIdx> <elemAddr> <modelId>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `modelId` - Mandatory. The target model of the target element.

	Bind an application key to a SIG defined Bluetooth mesh model.
	
- `model appkey unbind <addr> <appIdx> <elemAddr> <modelId>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `modelId` - Mandatory. The target model of the target element.

	Unbind an application key from a SIG defined Bluetooth mesh model.

- `model appkey get <addr> <elemAddr> <modeId>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.

	Get a list of the binded application key indexes for a SIG defined Bluetooth mesh model.
	
- `model appkey bind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `modelId` - Mandatory. The target model of the target element.
	- `cid` - Mandatory. The company identifier of the target vendor model.

	Bind an application key to a VENDOR defined Bluetooth mesh model.
	
- `model appkey unbind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `modelId` - Mandatory. The target model of the target element.
	- `cid` - Mandatory. The company identifier of the target vendor model.

	Unbind an application key from a VENDOR defined Bluetooth mesh model.

- `model appkey get-vnd <addr> <elemAddr> <modeId> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.

	Get a list of the binded application key indexes for a VENDOR defined Bluetooth mesh model.

### Model Publish Parameters Commands
Commands for configuring the publish parameters for Bluetooth mesh models within elements on nodes.

- `model publish get <addr> <elemAddr> <modelId>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.

	Get the publish parameters of a SIG defined Bluetooth mesh model.
	
- `model publish set <addr> <elemAddr> <modelId> <pubAddr> <pubAppIdx> <credFlag> <ttl> <periodKey> <period> <count> <interval>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `pubAddr` - Mandatory. Publish address for messages published by the model.
	- `pubAppIdx` - Mandatory. Application index used for encrypting messages published by the model.
	- `credFlag` - Mandatory. Friend credential flag for enabling/disabling friend feature for messages published by the model.
	- `ttl` - Mandatory. Time-to-live value for messages published by the model.
	- `periodKey` - Mandatory. Identifier for the units of the `period` argument that follows. Either 100ms, 1s, 10s, or 10m which represent 100 milliseconds, 1 second, 10 seconds, or 10 minutes respectively.
	- `period` - Mandatory. The publish period for the model in the units given in argument `periodKey`.
	- `count` - Mandatory. Retransmission count for messages published by the model (first transmission excluded).
	- `interval` - Mandatory. Retransmission interval for messages published by the model.

	Set the publish parameters of a SIG defined Bluetooth mesh model.
	
- `model publish get-vnd <addr> <elemAddr> <modelId> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.

	Get the publish parameters of a VENDOR defined Bluetooth mesh model.
	
- `model publish set <addr> <elemAddr> <modelId> <cid> <pubAddr> <pubAppIdx> <credFlag> <ttl> <periodKey> <period> <count> <interval>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.
	- `pubAddr` - Mandatory. Publish address for messages published by the model.
	- `pubAppIdx` - Mandatory. Application index used for encrypting messages published by the model.
	- `credFlag` - Mandatory. Friend credential flag for enabling/disabling friend feature for messages published by the model.
	- `ttl` - Mandatory. Time-to-live value for messages published by the model.
	- `periodKey` - Mandatory. Identifier for the units of the `period` argument that follows. Either 100ms, 1s, 10s, or 10m which represent 100 milliseconds, 1 second, 10 seconds, or 10 minutes respectively.
	- `period` - Mandatory. The publish period for the model in the units given in argument `periodKey`.
	- `count` - Mandatory. Retransmission count for messages published by the model (first transmission excluded).
	- `interval` - Mandatory. Retransmission interval for messages published by the model.

	Set the publish parameters of a VENDOR defined Bluetooth mesh model.

### Model Subscribe Address Commands
Commands for configuring the addresses subscribed to by Bluetooth mesh models within elements on nodes.

- `model subscribe add <addr> <elemAddr> <modelId> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `subAddr` - Mandatory. Destination address of messages which the model will subscribe to.

	Have a SIG defined Bluetooth mesh model subscribe to an address.
	
- `model subscribe delete <addr> <elemAddr> <modelId> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `subAddr` - Mandatory. Destination address of messages which the model will un-subscribe to.

	Have a SIG defined Bluetooth mesh model unsubscribe from an address.
	
- `model subscribe overwrite <addr> <elemAddr> <modelId> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `subAddr` - Mandatory. Destination address of messages which the model will subscribe to.

	Have a SIG defined Bluetooth mesh model unsubscribe from all currently subscribed addresses and then subscribe to a single new address.
	
- `model subscribe get <addr> <elemAddr> <modelId>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.

	Get a list of addresses that a SIG defined Bluetooth mesh model is currently subscribed to.
	
- `model subscribe add-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.
	- `subAddr` - Mandatory. Destination address of messages which the model will subscribe to.

	Have a VENDOR defined Bluetooth mesh model subscribe to an address.
	
- `model subscribe delete-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.
	- `subAddr` - Mandatory. Destination address of messages which the model will un-subscribe to.

	Have a VENDOR defined Bluetooth mesh model unsubscribe to an address
	
- `model subscribe overwrite-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.
	- `subAddr` - Mandatory. Destination address of messages which the model will subscribe to.

	Have a VENDOR defined Bluetooth mesh model unsubscribe from all currently subscribed addresses and then subscribe to a single new address.
	
- `model subscribe get-vnd <addr> <elemAddr> <modelId> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `elemAddr` - Mandatory. Address of the target element.
	- `modelId` - Mandatory. Model ID of the target model.
	- `cid` - Mandatory. The company identifier of the target vendor model.

	Get a list of addresses that a VENDOR defined Bluetooth mesh model is currently subscribed to.

## Model Message Commands
The `message` command set is used for interfaceing directly with mesh model messages using the gateway.

### Message Address Subscribe Commands
Commands for subscribing to and unsubscribing from mesh addresses for which destined messages should be received by the gateway and relayed to the user.

- `message subscribe <addr>`
	- `addr` - Mandatory. Mesh address for the gateway to subscribe to.

	Subscribe to a mesh address to receive messages destined for it.

- `message unsubscribe <addr>`
	- `addr` - Mandatory. Mesh address for the gateway to unsubscribe from.

	Unsubscribe from a mesh address to stop recieving  messages destined for it.
	
### Message Send Command
Send a mesh model message to a specific mesh address in the network.

- `message send <netIdx> <appIdx> <addr> <opcode> [payload]`
	- `netIdx` - Mandatory. Network index to encrypt the message with.
	- `appIdx` - Mandatory. Application index to encrypt the message with.
	- `addr` - Mandatory. The destination address for the message.
	- `opcode` - Mandatory. The opcode of the message.
	- `payload` - Optional. The payload of the message. Can be skipped for message opcodes that require no payload.

## Health Client Model Commands
The `health` command set utilises the gateway's health client model to interact with server client models on nodes within the network.

### Health Fault Commands
Commands for interfacing with node faults.

- `health fault get <addr> <appIdx> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.
	- `cid` - Mandatory. Company identifier to get faults for.

	Get a list of a node's current faults.

- `health fault clear <addr> <appIdx> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.
	- `cid` - Mandatory. Company identifier to clear faults for.

	Clear a node's current faults.
	
- `health fault test <addr> <appIdx> <cid>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.
	- `cid` - Mandatory. Company identifier to test faults for.

	Have a node run a fault test on itself.

### Health Period Commands
- `health period get <addr> <appIdx>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.

	Get a node's fast period divisor value used when faults are registered on the node.

	
- `health perios set <addr> <appIdx> <divisor>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.
	- `divisor` = Mandatory. Divisor applied to the health server's publish period used as period / (1 << divisor).

	Set a node's fast period divisor value used when faults are registered on the node.

### Health Attention Commands
- `health attention get <addr> <appIdx>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.

	Get a node's current attention timer value to see how much longer the node will bring attention to itself.
	
- `health attention set <addr> <appIdx> <attn>`
	- `addr` - Mandatory. Address of the target node.
	- `appIdx` - Mandatory. App index to encrypt with.
	- `attn` - Mandatory. Attention time to set (seconds).

	Set a node's attention timer value so that it brings attention to itself.

### Health Timeout Commands
- `health timeout get`

	Get the transmission timeout value for the gateway's health client model.

- `health timeout set <timeout>`
	- `timeout` - Mandatory. The timeout value for the gateways client model (milliseconds).

	Set the transmission timeout value for the gateway's health client model.
