# Nordic Mesh Gateway LTE JSON Message Definitions
NOTE: Values within ** are definitions of the variable type which the Gateway is expecting. All aother values are constants defined as such in this documentation.

## UNPROVISIONED DEVICE BEACON MESSAGES
### Unprovisioned Mesh Device Beacon Request - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "beacon_request"
    }
}
~~~

### Unprovisioned Mesh Device Beacon List - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "beacon_list",
        "timestamp": "*string*",
        "beacons": [
            {
                "deviceType": "BT-Mesh",
                "uuid": "*hexadecimal string of 128-bit integer*",
                "oobInfo": "*string*",
                "uriHash": *unsinged 32-bit integer*
            }
        ]
    },
    "messageId": "*integer*"
}
~~~

## PROVISION DEVICES
### Provision Device - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "provision",
        "uuid": "*hexadecimal string of 128-bit integer*",
        "netIndex": *unsigned 16-bit integer*,
        "address": *unsigned 16-bit integer*,
        "attention": *unsigned 8-bit integer*
        }
    }
}
~~~

### Provision Result - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "provision_result",
        "timestamp": "*string*",
        "error": *integer*,
        "uuid": "*hexadecimal string of 128-bit integer*",
        "netIndex": "*unsigned 16-bit integer*",
        "address": "*unsigned 16-bit integer*",
        "elementCount": "*unsigned 8-bit integer*"
    },
    "messageId": "*integer*"
}
~~~

### Reset Node - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operaetion": {
        "type": "reset",
        "address": *unsigned 16-bit integer*
    }
}
~~~

### Reset Node Result - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "reset_result",
        "error": *integer*
    },
    "messageId": *integer*
}
~~~

## MESH NETWORK SUBNET CONFIGURATION
### Add Subnet to Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "subnet_add",
        "netKey": "*hexadecimal string of 128-bit integer*",
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

### Generate New Subnet for Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "subnet_generate",
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

### Delete Subnet from Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "subnet_delete",
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

### Request Subnets of Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "opereation",
    "operation": {
        "type": "subnet_request"
    }
}
~~~

### List of Subnets of Mesh Network - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "subnet_list",
        "timestamp": "*string*",
        "subnetList": [
            {
                "netIndex": *unsigned 16-bit integer*
            }
        ]
    },
    "messageId": *integer*
}
~~~

## MESH NETWORK APPLICATION KEY CONFIGURATION
### Add Application Key to Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "app_key_add",
        "appKey": "*hexadecimal string of 128-bit integer*",
        "appIndex": *unsigned 16-bit integer*,
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

### Generate New Application Key for Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "app_key_generate",
        "appIndex": *unsigned 16-bit integer*,
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

### Delete Application Key from Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "app_key_delete",
        "appIndex": *unsigned 16-bit integer*
    }
}
~~~

### Request Application Keys of Mesh Network - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "app_key_request"
    }
}
~~~

### List of Application Keys of Mesh Network - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "app_key_list",
        "timestamp": "*string*",
        "appKeyList": [
            {
                "appIndex": *unsigned 16-bit integer*,
                "netIndex": *unsigned 16-bit integer*
            }
        ]
    },
    "messageId": "*integer*"
}
~~~

## MESH NETWORK NODES
### Node Request - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_request"
    }
}
~~~

### Node List - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "node_list",
        "timestamp": "*string*",
        "nodes": [
            {
                "deviceType": "BT-Mesh",
                "uuid": "*hexadecimal string of 128-bit integer*",
                "netIndex": *unsigned 16-bit integer*,
                "address": *unsigned 16-bit integer*,
                "elementCount": *unsigned 8-bit integer*
            }
        ]
    },
    "messageId": *integer*
}
~~~

### Node Discover - Cloud to Gateway
~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_discover",
        "address": unsigned 16-bit integer
    }
}
~~~

### Discover Node Result - Gateway to Cloud
~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "node_discover_result",
        "timeStamp": "*string ISO 8601*",
        "error": *integer*,
        "status": *unsigned 8-bit integer*,
        "address": *unsigned 16-bit integer*,
        "uuid": "*string*",
        "companyId": *unsigned 16-bit integer*,
        "pid": *unsigned 16-bit integer*,
        "vid": *unsigned 16-bit integer*,
        "crpl": *unsigned 16-bit integer*,
        "networkBeaconState": *boolean*,
        "timeToLive": *unsigned 8-bit integer*,
        "relayFeature": {
            "suport": *boolean*,
            "state": *boolean*,
            "retransmitCount": *unsigned 8-bit integer*,
            "retransmitInterval": *unsigned 16-bit integer*
        },
        "proxyFeature": {
            "support": *boolean*,
            "state": *boolean*
        },
        "friendFeature": {
            "support": *boolean*,
            "state": *boolean*
        },
        "lpnFeature": {
            "state": *boolean*
        },
	"heartbeatSubscribe" : {
		"sourceAddress": *unsigned 16-bit integer*,
		"destinationAddress": *unsigned 16-bit integer*,
		"period": *unsigned 8-bit integer*,
		"count": *unsigned 8-bit integer*,
		"minimumHops": *unsigned 8-bit integer*,
		"maximumHops": *unsigned 8-bit integer*
	},
	"heartbeatPublish": " {
		"destinationAddress": *unsigned 16-bit integer*,
		"count": *unsigned 8-bit integer*,
		"period": *unsigned 8-bit integer*,
		"timeToLive": *unsigned 8-bit integer*,
		"relayFeature": *bool*,
		"proxyFeature": *bool*,
		"friendFeature": *bool*,
		"lpnFeature": *bool*,
		"netIndex": *unsigned 16-bit integer*
	},
        "subnets": [
            *unsigned 16-bit integer*
        ],
        "elements": [
            {
                "address": *unsigned 16-bit integer*,
                "loc": *unsigned 16-bit integer*,
                "sigModels": [
                    {
                        "modelId": *unsigned 16-bit integer*,
                        "appIndexes": [
                            *unsigned 16-bit integer*
                        ],
                        "subscribeAddresses": [
                            *unsigned 16-bit integer*
                        ],
                        "publishParameters": {
                            "address": *unsigned 16-bit integer*,
                            "appIndex": *unsigned 16-bit integer*,
                            "friendCredentialFlag": *boolean*,
                            "timeToLive": *unsigned 8-bit integer*,
                            "period": *unsigned 8-bit integer*,
                            "periodUnits": *string*,
                            "retransmitCount": *unsigned 8-bit integer*,
                            "retransmitInterval": *unsigned 8-bit integer*
                        }
                    }
                ],
                "vendorModels": [
                    {
                        "companyId": unsigned 16-bit integer,
                        "modelId": unsigned 16-bit integer,
                        "appIndexes": [
                            *unsigned 16-bit integer*
                        ],
                        "subscribeAddresses": [
                            *unsigned 16-bit integer*
                        ],
                        "publishParameters": {
                            "appIndex": *unsigned 16-bit integer*,
                            "friendCredentialFlag": *boolean*,
                            "timeToLive": *unsigned 8-bit integer*,
                            "period": *unsigned 8-bit integer*,
                            "periodUnits": "*string*",
                            "retransmitCount": *unsigned 8-bit integer*,
                            "retransmitInterval": *unsigned 8-bit integer*
                        }
                    }
                ]
            }
        ]
    },
    "messageId": *integer*
}
~~~

### Configure Node Messages - Cloud to Gateway
#### Set Network Beacon
Enable or disable a node's network beacon. 

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "networkBeaconSet",
        "address": *unsigned 16-bit integer*,
        "state": *boolean*
    }
}
~~~

#### Set Time to Live
Set a node's time-to-live value.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "timeToLiveSet",
        "address": *unsigned 16-bit integer*,
        "timeToLive": *unsigned 8-bit integer*
    }
}
~~~

#### Set Relay Feature
Enable or disable a node's realy feature.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "relayFeatureSet",
        "address": *unsigned 16-bit integer*"
        "state": *boolean*,
        "retransmitCount": *unsigned 8-bit integer*,
        "retransmitInterval": *unsigned 16-bit integer*
    }
}
~~~

#### Set Proxy Feature
Enable or disable a node's proxy feature.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "proxyFeatureSet",
        "address": *unsigned 16-bit integer*,
        "state": *boolean*
    }
}
~~~

#### Set Friend Feature
Enable or disable a node's friend feature.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "friendFeatureSet",
        "address": *unsigned 16-bit integer*,
        "state": *boolean*
    }
}
~~~

#### Add Subnet
Add a subnet's network key to a node.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subnetAdd",
        "address": *unsigned 16-bit integer*,
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

#### Delete Subnet
Remove a subnet's network key from a node.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subnetDelete",
        "address": *unsigned 16-bit integer*,
        "netIndex": *unsigned 16-bit integer*
    }
}
~~~

#### Bind Application Key
Bind an application key to a SIG defined model on a node.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "appKeyBind",
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*
    }
}
~~~

Bind an application key to a VENDOR defined model on a node.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "appKeyBindVnd,
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "companyId": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*
    }
}
~~~

#### Unbind Application Key
Unbind an application key from a SIG defined model.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "appKeyUnbind",
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*
    }
}
~~~

Unbind an application key from a VENDOR defined model.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "appKeyUnbindVnd",
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "companyId": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*
    }
}
~~~

#### Set Publish Parameters
Set the publish parameters for a SIG defined model.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "publishParametersSet",
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modeId": *unsigned 16-bit integer*,
        "publishAddress": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*,
        "friendCredentialFlag": *bool*,
        "timeToLive": *unsigned 8-bit integer*,
        "period": *unsigned 8-bit integer*,
        "periodUnits": "*string*",
        "retransmitCount": *unsigned 8-bit integer*,
        "retransmitInterval": *unsigned 16-bit integer*
    }
}
~~~

Set the publish parameters for a VENDOR defined model.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "publishParametersSetVnd",
        "address": *unsigned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modeId": *unsigned 16-bit integer*,
        "companyId": *unsigned 16-bit integer*,
        "publishAddress": *unsigned 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*,
        "friendCredentialFlag": *bool*,
        "timeToLive": *unsigned 8-bit integer*,
        "period": *unsigned 8-bit integer*,
        "periodUnits": "*string*",
        "retransmitCount": *unsigned 8-bit integer*,
        "retransmitInterval": *unsigned 16-bit integer*
    }
}
~~~

#### Add subscribe Address
Add an address to a SIG defined model's subscribe list.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressAdd",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

Add an address to a VENDOR defined model's subscribe list.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressAddVnd",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "companyId": *unsigned 16-bit integer*
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

#### Delete Subscribe Address
Remove an address from a SIG defined model's subscribe list.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressDelete",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

Remove an address from a VENDOR defined model's subscribe list.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressDeleteVnd",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit integer*,
        "modelId": *unsigned 16-bit integer*,
        "companyId": *unsigned 16-bit integer*,
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

#### Overwrite Subscribe Addresses
Overwrite all addresses on a SIG defined model's subscribe list with one new address.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressOverwrite",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit address*,
        "modelId": *unsigned 16-bit address*,
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

Overwrite all addresses on a VENDOR defined model's subscribe list with one new address.

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "node_configure",
        "configuration": "subscribeAddressOverwriteVnd",
        "address": *unisgned 16-bit integer*,
        "elementAddress": *unsigned 16-bit address*,
        "modelId": *unsigned 16-bit address*,
        "companyId": *unsigned 16-bit integer*,
        "subscribeAddress": *unsigned 16-bit integer*
    }
}
~~~

#### Set Heartbeat Subscribe Parameters
Set a node's heartbeat subscribe parameters.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "node_configuration",
		"configuration": "heartbeatSubscribeSet",
		"address": "*unsigned 16-bit integer*",
		"sourceAddress": "*unsigned 16-bit integer*",
		"destinationAddress": "*unsigned 16-bit integer*",
		"period": "*unsigned 8-bit itneger*",
		"count": "*unsigned 8-bit integer*",
		"minimumHops": "*unsigned 8-bit integer*",
		"maximumHops": "*unsigned 8-bit integer*"
	}
}
~~~

#### Set Node Heartbeat Publish Parameters
Set a node's heartbeat publish parameters.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "configuration",
		"configuration": heartbeatPublishSet",
		"address": "*unsigned 16-bit integer*",
		"netIndex": "*unsigned 16-bit integer*",
		"destinationAddress": "*unsigned 16-bitinteger*",
		"count": "*unsigned 8-bit integer*",
		"period": "*unsigned 8-bit integer*",
		"timeToLive": "*unsigned 8-bit integer*",
		"relayFeature": "*bool*",
		"proxyFeature": "*bool*",
		"friendFeature": "*bool*",
		"lpnFeature": "*bool*
	}
}
~~~

## MESH MESSAGE SUBSCRIBE
### Subscribe to Mesh Messages - Cloud to Gateway

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "subscribe",
        "address_list": [
            {
                "address": *unsigned 16-bit integer*
            }
        ]
    }
}
~~~

### Unsubscribe to Mesh Messages - Cloud to Gateway

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "unsubscribe",
        "addressList": [
            {
                "address": *unsigned 16-bit integer*
            }
        ]
    }
}
~~~

### Get Subscribe List - Cloud to Gateway

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "subscribe_list_request"
    }
}
~~~

### Subscribe List - Gateway to Cloud

~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "subscribe_list",
        "addressList": [
            {
                "address": *unsigned 16-bit integer*
            }
        ]
    }
}
~~~

## MESH MESSAGES
### Send Mesh Model Message - Cloud to Gateway

~~~json
{
    "id": "*string*",
    "type": "operation",
    "operation": {
        "type": "send_model_message",
        "netIndex": *unsinged 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*,
        "address": *unsigned 16-bit itneger*,
        "opcode": *unsigned 32-bit integer*,
        "payload": [
                {
                        "byte": *unisgned 8-bit integer*
                }
        ]
    }
}
~~~

### Receive Mesh Model Message - Gateway to Cloud

~~~json
{
    "type": "event",
    "gatewayId": "*string*",
    "event": {
        "type": "receive_model_message",
        "netIndex": *unsinged 16-bit integer*,
        "appIndex": *unsigned 16-bit integer*,
        "sourceAddress": *unsigned 16-bit integer*,
        "destinationAddress": *unsigned 16-bit integer*,
        "opcode": *unsigned 32-bit integer*,
        "payload": [
                {
                        "byte": *unsigned 8-bit integer*
                }
        ]
    }
}
~~~ 

## HEALTH MODEL MESSAGES
### Get Node Health Faults Message - Cloud to Gateway
Get the registered faults from a node.

~~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_fault_get",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"companyId": *unsigned 16-bit integer*
	}
}
~~~

### Clear Node Health Faults Message - Cloud to Gateway
Clear a list of a node's registered faults.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_fault_clear",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"companyId": *unsigned 16-bit integer*
	}
}
~~~

### Test Node Health Faults Message - Cloud to Gateway
Run a fault test on a node.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_fault_test",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"companyId": *unsigned 16-bit integer*,
		"testId":"*unsigned 8-bit integer*"
	}
}
~~~

### Node Health Current Faults Message - Gateway to Cloud
A list of a node's current faults.

~~~json
{
	"type": "event",
	"gatewayId": "*string*",
	"event": {
		"type": "health_faults_current",
		"address": *unsigned 16-bit integer*,
		"testId": *unsigned 8-bit integer*,
		"comapnyId": *unsigned 16-bit integer*,
		"faults": [
			{
				"fault": *unsigned 8-bit integer*
			}
		]
	}
}
~~~

### Node Health Registered Faults Message - Gateway to Cloud
A list of a node's registered faults.

~~~json
{
	"type": "event",
	"gatewayId": "*string*",
	"event": {
		"type": "health_faults_registered",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"companyId": *unsigned 16-bit integer*,
		"testId": *unsigned 8-bit integer*,
		"faults": [
			{
				"fault": *unsigned 8-bit integer*
			}
		]
	}
}
~~~

### Node Health Period Get Message - Cloud to Gateway
Get a node's health faults publish period (fast-period) for when faults are present.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_period_get",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*
	}
}
~~~

### Node Health Period Set Message - Cloud to Gateway
Set a node's health faults publish period (fast-period) for when faults are present.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_period_set",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"divisor": *unsigned 8-bit integer*"
	}
}
~~~

### Node Health Period Message - Gateway to Cloud
A node's current health fault publish period (fast-period) for when faults are present.

~~~json
{
	"type": "event",
	"gatewayId": "*string*",
	"event": {
		"type": "health_period",
		"address": *unsigned 16-bit integer*,
		"divisor": *unsigned 8-bit integer*
	}
}
~~~

### Node Health Attention Get Message - Cloud to Gateway
Get a node's attention timer value.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_attention_get",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
	}
}
~~~

### Node Health Attention Set Message - Cloud to Gateway
Set a node's attention timer value.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_attention_set",
		"address": *unsigned 16-bit integer*,
		"appIndex": *unsigned 16-bit integer*,
		"attention": *unsigned 8-bit integer*
	}
}
~~~

### Node Health Attention Message - Gateway to Cloud
A node's attention timer value.

~~~json
{
	"type": "event",
	"gatewayId": "*string*",
	"event": {
		"type": "health_attention",
		"address": *unsigned 16-bit integer*,
		"attention": *unsigned 8-bit integer*
	}
}
~~~

### Gateway Health Client Timeout Get Message - Cloud to Gateway
Get the gateway's health client imeout value.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_client_timeout_get"
	}
}
~~~

### Gateway Health Client Timeout Set Message - Cloud to Gateway
Set the gateway's health client timeout value.

~~~json
{
	"id": "*string*",
	"type": "operation",
	"operation": {
		"type": "health_client_timeout_set",
		"timeout": *32-bit integer*
	}
}
~~~

### Gateway Health Client Timeout Message - Gateway to Cloud
The gateway's health client timeout value.

~~~json
{
	"type": "event",
	"gatewayId": "*string*",
	"event": {
		"type": "health_client_timeout",
		"timeout": *32-bit integer*
	}
}
~~~
