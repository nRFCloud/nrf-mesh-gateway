#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>
#include <shell/shell_uart.h>

/* Include mesh stack header for direct access to mesh access layer messaging. */
#include "mesh/net.h"
#include "mesh/access.h"

#include "btmesh.h"
#ifdef CONFIG_SHELL
#include "cli.h"
#endif
#include "util.h"

#define MAX_PAYLOAD_LEN 32

static const struct shell *shell;

static bool str_to_state(char *str, bool *val)
{
        uint8_t i;

        for (i = 0; str[i]; i++) {
                str[i] = tolower(str[i]);
        }

        if (!strcmp(str, "true")) {
                *val = true;
                return true;
        }

        if (!strcmp(str, "false")) {
                *val = false;
                return true;
        }

        if (!strcmp(str, "enable")) {
                *val = true;
                return true;
        }

        if (!strcmp(str, "disable")) {
                *val = false;
                return true;
        }

        if (!strcmp(str, "0")) {
                *val = false;
                return true;
        }

        if (!strcmp(str, "1")) {
                *val = true;
                return true;
        }

        return false;
}

static bool str_to_long(char *str, long *number)
{
        uint8_t i;

        if (!strncmp(str, "0x", 2)) {
                /* String is hexidecimal number */
                i = 2;
                while (str[i] == '0') {
                        i++;
                }
                if (str[i] == '\0') {
                        /* Number is a legitimate 0 */
                        *number = 0;
                } else {
                        *number = strtoul(&str[2], NULL, 16);
                        if (*number == 0) {
                                /* Number is 0 because strtoul failed */
                                return false;
                        }
                }
        } else {
                /* String is decimal number */
                i = 0;
                while (str[i] == '0') {
                        i++;
                }
                if (str[i] == '\0') {
                        /* Number is a ligitimate 0 */
                        *number = 0;
                } else {
                        *number = strtoul(str, NULL, 10);
                        if (*number == 0) {
                                /* Number is 0 because strtoul failed */
                                return false;
                        }
                }
        }

        return true;
}

static bool str_to_uint8(char *str, uint8_t *uint8)
{
        long number;

        if (!str_to_long(str, &number)) {
                return false;
        } 

        /* Number can't be larger than a 8-bit unsigned */
        if (number > UINT8_MAX) {
                return false;
        }

        *uint8 = (uint8_t)number;
        return true;
}

static bool str_to_uint16(char *str, uint16_t *uint16)
{
        long number;

        if (!str_to_long(str, &number)) {
                return false;
        }    

        /* Number can't be larger than a 16-bit unsigned */
        if (number > UINT16_MAX) {
                return false;
        }

        *uint16 = (uint16_t)number;
        return true;
}

static bool str_to_uint32(char *str, uint32_t *uint32)
{
        long number;

        if (!str_to_long(str, &number)) {
                return false;
        }

        /* Number can't be larger than a 32-bit unsigned */
        if (number > UINT32_MAX) {
                return false;
        }

        *uint32 = (uint32_t)number;
        return true;
}


/******************************************************************************
 *  PROVISION COMMANDS
 *****************************************************************************/
static int provision_cmd(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint8_t i;
        bool arg_addr;
        bool arg_net_idx;
        bool arg_attn;
        union btmesh_op_args op_args;

        /* Set defaults */
        arg_addr = false;
        arg_net_idx = false;
        arg_attn = false;
        op_args.prov_adv.addr = 0x00;
        op_args.prov_adv.net_idx = 0x00;
        op_args.prov_adv.attn = 0x00;

        if (argc % 2 == 0) {
                shell_error(shell, "Optional arguments must be paried with a valid key: -i, -a, -t");
                return -EINVAL;
        }

        /* Get UUID from argv */
        util_str2uuid(argv[0], op_args.prov_adv.uuid);

        for (i = 1; i < argc - 1; i=i+2) {
                if (!arg_addr && !strcmp(argv[i], "-a")) {
                        /* Check for addr optional arg */
                        arg_addr = true;

                        /* Get address from argv */
                        if (!str_to_uint16(argv[i+1], &(op_args.prov_adv.addr))) {
                                shell_error(shell, "Address argument invalid\n");
                                return -EINVAL;
                        }

                        continue;
                } else if (!arg_net_idx && !strcmp(argv[i], "-i")) {
                        /* Check for net_idx optional arg */
                        arg_net_idx = true;

                        /* Get net index from argv */
                        if (!str_to_uint16(argv[i+1], &(op_args.prov_adv.net_idx))) {
                                shell_error(shell, "Net index argument invalid\n");
                                return -EINVAL;
                        }

                        continue;
                } else if (!arg_attn && !strcmp(argv[i], "-t")) {
                        /* Check for attn optional arg */
                        arg_attn = true;

                        /* Get attention time from argv */
                        if (!str_to_uint8(argv[i+1], &(op_args.prov_adv.attn))) {
                                shell_error(shell, "Attention time argument invalid\n");
                                return -EINVAL;
                        }
                        if (op_args.prov_adv.attn > UINT8_MAX) {
                                shell_error(shell, "Attention time argument larger than 255 seconds\n");
                                return -EINVAL;
                        }

                        continue;
                }

                shell_error(shell, "Invalid argument key: %s. Valid keys include -i, -a, -t\n", argv[i]);
                return -EINVAL;
        }

        err = btmesh_perform_op(BTMESH_OP_PROV_ADV, &op_args);
        if (err) {
                shell_error(shell, "Failed to provision device. Error Code: %d", err);
                return -ENOEXEC;
        }

        return 0;
}

#define DYNAMIC_PROV_HELP "Valid UUID\n"

static void get_dynamic_prov(size_t idx, struct shell_static_entry *entry)
{
        entry->syntax = btmesh_get_beacon_uuid(idx);

        if (entry->syntax == NULL) {
                return;
        }

        entry->handler = provision_cmd;
        entry->subcmd = NULL;
        entry->help = DYNAMIC_PROV_HELP;
        entry->args.mandatory = 1;
        entry->args.optional = 6;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_prov, get_dynamic_prov);

#define PROV_HELP \
        "Provision a Bluetooth mesh devices.\n" \
        "USAGE:\n" \
        " provision <UUID> [-i netIdx] [-a addr] [-t attn]\n" \
        " - UUID   : 128-bit UUID represented by 32 hexadecimal characters.\n" \
        " - netIdx : Unsigned 16-bit network index to provision device under.\n" \
        " - addr   : Unsigned 16-bit address to provision the device to.\n" \
        " - attn   : Attention timer for the device being provisioned.\n" \
        " * If no netIdx argument is provided, netIdx of 0x00 will be used.\n" \
        " * If no addr argument is provided or an addr argument of 0 is provided, the lowest available address will be used.\n" \
        " * If no attn argument is provided, an attention time of 0 seconds will be used."

SHELL_CMD_REGISTER(provision, &dynamic_prov, PROV_HELP, NULL);


/******************************************************************************
 *  DATABASE SUBNET COMMANDS
 *****************************************************************************/
#define DYNAMIC_SUBNET_VIEW_HELP "Valid Network Index"
#define NET_GET_ERR "Failed to get subnet with provided network index from storage."
#define ARG_NUM_ERR "Invalid number of arguments. Optional arguments must be proceded by a valid key: "
#define ARG_KEY_ERR "Invalid optional argument key. Only the following keys are vaild: "
#define NET_KEY_ERR "Invalid netKey. Network keys must be 32 hexadecimal characters."
#define NET_IDX_ERR "Invalid netIdx. Network indexes must be 16-bit unsigned integers."
#define NET_IDX_DUP_ERR "Subnet with netIdx already exists. The subnet key must first be removed before another key can be added under the same index."
#define NET_ALLOC_ERR "Failed to allocate subnet: subnet storage full. A subnet must be removed before another can be allocated."
#define NET_IDX_RANGE_ERR "Invalid netIdx. Only values within range 0x0001-0xfffe are vaild network indexes."

static char subnet_strings[SUBNET_COUNT][7];

static int subnet_list(const struct shell *shell, size_t argc, char **argv)
{
        ARG_UNUSED(argc);
        ARG_UNUSED(argv);

        uint8_t i;
        uint8_t j;
        uint8_t idx;
        uint16_t lowest_idx;
        uint16_t last_lowest_idx;
        char net_key_str[33];
        struct bt_mesh_cdb_subnet *subnet_ptr;

        shell_info(shell, "Net Index: Net Key");

        idx = 0;
        lowest_idx = 0xFFFF;
        last_lowest_idx = 0x0000;
        subnet_ptr = bt_mesh_cdb.subnets;
        for (i = 0; i < SUBNET_COUNT; i++) {
                for (j = 0; j < SUBNET_COUNT; j++) {
                        if ((subnet_ptr[j].net_idx < lowest_idx)) {
                                if ((i == 0) || (subnet_ptr[j].net_idx > last_lowest_idx)) {
                                        lowest_idx = subnet_ptr[j].net_idx;
                                        idx = j;
                                }
                        }
                }

                if (lowest_idx == BT_MESH_KEY_UNUSED) {
                        shell_print(shell, "");
                        return 0;
                }

                util_key2str(subnet_ptr[idx].keys[subnet_ptr[idx].kr_phase].net_key, net_key_str);
                shell_print(shell, "  0x%04x : %s", subnet_ptr[idx].net_idx, net_key_str); 

                last_lowest_idx = lowest_idx;
                lowest_idx = 0xFFFF;
        }

        shell_print(shell, "");
        return 0;
}

static int subnet_view(const struct shell *shell, size_t argc, char **argv)
{
        uint16_t net_idx;
        char net_key_str[KEY_STR_LEN];
        struct bt_mesh_cdb_subnet *subnet_ptr;

        if (!str_to_uint16(argv[0], &net_idx)) {
                shell_error(shell, "%s: %s\n", argv[0], NET_IDX_ERR);
                return -EINVAL;
        }

        subnet_ptr = bt_mesh_cdb_subnet_get(net_idx);
        if (subnet_ptr == NULL) {
                shell_error(shell, "%s: %s\n", argv[0], NET_GET_ERR);
                return -ENOEXEC;
        }
        util_key2str(subnet_ptr->keys[subnet_ptr->kr_phase].net_key, net_key_str);

        shell_info(shell, "Subnet Index : Subnet Key");
        shell_print(shell, "      0x%04x : %s\n", net_idx, net_key_str);

        return 0;
}

static void get_dynamic_subnets(size_t idx, struct shell_static_entry *entry)
{
        uint8_t i;
        uint8_t j;
        uint16_t lowest_idx;
        uint16_t last_lowest_idx;
        struct bt_mesh_cdb_subnet *subnet_ptr;

        /* If this is the first time through, create and sort a list of net indexes */
        lowest_idx = 0x0000;
        last_lowest_idx = 0x0000;
        subnet_ptr = bt_mesh_cdb.subnets;
        if (idx == 0) {
                for (i = 0; i < SUBNET_COUNT; i++)
                {
                        for (j = 0; j < SUBNET_COUNT; j++) {
                                if ((subnet_ptr[j].net_idx < lowest_idx) && (subnet_ptr[j].net_idx > last_lowest_idx)) {
                                        lowest_idx = subnet_ptr[j].net_idx;
                                }
                        }
                        snprintf(subnet_strings[i], sizeof(subnet_strings[i]), "0x%04x", lowest_idx);
                        last_lowest_idx = lowest_idx;
                        lowest_idx = 0xFFFF;
                }
        }

        if (!strcmp(subnet_strings[idx], "0xffff")){
                entry->syntax = NULL;
                return;
        }

        entry->syntax = subnet_strings[idx];
        entry->help = DYNAMIC_SUBNET_VIEW_HELP;
}

static void get_dynamic_subnet_view(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = subnet_view;
        entry->subcmd = NULL;
        entry->args.mandatory = 1;
        entry->args.optional = 0;
}

static int get_net_idx(const struct shell *shell, char **arg, uint16_t *net_idx, bool opt)
{
        char *net_idx_ptr;

        /* If the net_idx is an optional argument it must have an argument key infrnt of it */
        if (opt) {
                if (strcmp(arg[0], "-i")) {
                        shell_error(shell, "%s: %s\n", arg[0], ARG_KEY_ERR);
                        return -EINVAL;
                }
                net_idx_ptr = arg[1];
        } else {
                net_idx_ptr = arg[0];
        }

        if (!str_to_uint16(net_idx_ptr, net_idx)) {
                shell_error(shell, "%s: %s\n", net_idx_ptr, NET_IDX_ERR);
                return -EINVAL;
        }

        if (*net_idx == 0xFFFF) {
                shell_error(shell, "%s: %s\n", net_idx_ptr, NET_IDX_RANGE_ERR);
                return -EINVAL;
        }

        return 0;
}

static void get_lowest_net_idx(uint16_t *net_idx)
{
        /* Find lowest net index to use */
        *net_idx = 0x01; /* Primary network index 0x00 will never be available */
        while(bt_mesh_cdb_subnet_get(*net_idx) != NULL) {
                *net_idx = *net_idx + 1;
        }
}

static int create_subnet(const struct shell *shell, uint16_t net_idx, uint8_t net_key[KEY_LEN])
{
        uint8_t err;
        struct bt_mesh_cdb_subnet *subnet_ptr;

        /* Allocate new subnet in CDB */
        subnet_ptr = bt_mesh_cdb_subnet_alloc(net_idx);
        if (subnet_ptr == NULL) {
                shell_error(shell, "%s\n", NET_ALLOC_ERR);
                return -ENOEXEC;
        }

        /* Add network key to subnet and store in CDB */
        memcpy(subnet_ptr->keys[0].net_key, net_key, KEY_LEN);
        bt_mesh_cdb_subnet_store(subnet_ptr);

        err = bt_mesh_subnet_add(net_idx, net_key);

        if (err) {
                bt_mesh_cdb_subnet_del(subnet_ptr, true); 
                return -ENOEXEC;
        }

        return 0;
}

static int subnet_add(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint8_t net_key[KEY_LEN];
        uint16_t net_idx;

        /* Must have either 1 mandatory argument or 1 mandatory argument, 1
         * optional key and 1 optional argument */
        if (argc % 2 != 0) {
                shell_error(shell, "%s-i\n", ARG_NUM_ERR);
                return -EINVAL;
        }

        /* Make sure the key argument is valid */
        if (!util_key_valid(argv[1])) {
                shell_error(shell, "%s: %s\n", argv[1], NET_KEY_ERR);
                return -EINVAL;
        }
        util_str2key(argv[1], net_key);

        /* If the optional netIdx argument was provided... */
        if (argc > 2) {     /* User has specified net index to use */
                err = get_net_idx(shell, &argv[2], &net_idx, true);
                if (err) {
                        return err;
                }

                /* Make sure the requested net index is available */ 
                if (bt_mesh_cdb_subnet_get(net_idx) != NULL) {
                        shell_error(shell, "%s: %s\n", argv[1], NET_IDX_DUP_ERR);
                        return -EINVAL; 
                }
        } else {    /* No netIdx argument was provided, must use lowest available index */
                get_lowest_net_idx(&net_idx);
        }

        err = create_subnet(shell, net_idx, net_key);
        if (err) {
                return err;
        }

        shell_info(shell, "Successfully added network key");
        shell_print(shell, "Network Key: %s", argv[1]);
        shell_print(shell, "Network Index: 0x%04x\n", net_idx);
        return 0;
}

static int subnet_generate(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint8_t net_key[KEY_LEN];
        char net_key_str[KEY_STR_LEN];
        uint16_t net_idx;

        /* Must have either no arguments or 1 optional argument key + 1 optional argument */
        if (argc % 2  == 0) {
                shell_error(shell, "%s-i\n", ARG_NUM_ERR);
                return -EINVAL;
        }

        if (argc > 1) { /* User has specified net index to use */
                err = get_net_idx(shell, &argv[1], &net_idx, true);
                if (err) {
                        return err;
                }

                /* Make sure the requested net index is available */ 
                if (bt_mesh_cdb_subnet_get(net_idx) != NULL) {
                        shell_error(shell, "%s: %s\n", argv[1], NET_IDX_DUP_ERR);
                        return -EINVAL; 
                }
        } else { /* No netIdx argument was provided, must find lowest available index */
                get_lowest_net_idx(&net_idx);
        }

        /* Generate network key */
        bt_rand(net_key, KEY_LEN);
        err = create_subnet(shell, net_idx, net_key);
        if (err) {
                return err;
        }

        util_key2str(net_key, net_key_str);
        shell_info(shell, "Successfully generated network key");
        shell_print(shell, "Network Key: %s", net_key_str);
        shell_print(shell, "Network Index: 0x%04x\n", net_idx);
        return 0;
}

static int subnet_remove(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint16_t net_idx;
        struct bt_mesh_cdb_subnet *subnet_ptr;

        err = get_net_idx(shell, argv, &net_idx, false);
        if (err) {
                return err;
        }

        subnet_ptr = bt_mesh_cdb_subnet_get(net_idx);
        if (subnet_ptr == NULL) {
                shell_error(shell, "%s: %s\n", argv[0], NET_GET_ERR);
                return -ENOEXEC;
        }

        bt_mesh_cdb_subnet_del(subnet_ptr, true); 
        bt_mesh_subnet_del(net_idx);
        return 0;
}

static void get_dynamic_subnet_remove(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = subnet_remove;
        entry->subcmd = NULL;
        entry->args.mandatory = 1;
        entry->args.optional = 0;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_subnet_view, get_dynamic_subnet_view);
SHELL_DYNAMIC_CMD_CREATE(dynamic_subnet_rem, get_dynamic_subnet_remove);

#define SUBNET_LIST_HELP \
        "List all network index:key pairs stored in the gateway.\n" \
"USAGE:\n" \
"subnet list\n"
#define SUBNET_VIEW_HELP \
        "View a stored network key by entering the associated network index.\n" \
"USAGE:\n" \
"subnet view <netIdx>\n" \
"- netIdx: The network index associated with the network key to be viewed.\n"
#define SUBNET_ADD_HELP \
        "Add a new network key to the gateway.\n" \
"USAGE:\n" \
"subnet add <netKey> [-i netIdx]\n" \
"- netKey: 128-bit network key represented by 32 hexadecimal characters.\n" \
"- netIdx: Unsigned 16-bit network index to associate with the network key.\n" \
" * If no netIdx argument is provided, the lowest available network index will be used.\n"
#define SUBNET_GEN_HELP \
        "Generate a new network key.\n" \
"USAGE:\n" \
"subnet generate [-i netIdx]\n" \
"- netIdx: Unsigned 16-bit network index to associate with the generated network key.\n" \
" * If no netIdx argument is provided, the lowest available network index will be used.\n"
#define SUBNET_REM_HELP \
        "Remove a network key associated with a specific network index.\n" \
"USAGE:\n" \
"subnet remove <netIdx>\n" \
"- netIdx: Unsigned 16-bit network index associated with the network key to be removed.\n"

SHELL_STATIC_SUBCMD_SET_CREATE(subnet_subs,
                SHELL_CMD_ARG(list, NULL, SUBNET_LIST_HELP, subnet_list, 1, 0),
                SHELL_CMD(view, &dynamic_subnet_view, SUBNET_VIEW_HELP, NULL),
                SHELL_CMD_ARG(add, NULL, SUBNET_ADD_HELP, subnet_add, 2, 2),
                SHELL_CMD_ARG(generate, NULL, SUBNET_GEN_HELP, subnet_generate, 1, 2),
                SHELL_CMD(remove, &dynamic_subnet_rem, SUBNET_REM_HELP, NULL),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(subnet, &subnet_subs, "Sub-network database interface.",
                NULL);


/******************************************************************************
 *  DATABASE APPKEY COMMANDS
 *****************************************************************************/
#define DYNAMIC_APPKEY_VIEW_HELP "Valid Application Index"
#define APP_GET_ERR "Failed to get application key with provided application index from storage."
#define APP_KEY_ERR "Invalid appKey. Application keys must be 32 hexadecimal characters."
#define APP_IDX_ERR "Invalid appIdx. Application indexes must be 16-bit unsigned integers."
#define APP_IDX_DUP_ERR "Application key with appIdx already exists. The application key must first be removed before another key can be added under the same index."
#define APP_IDX_RANGE_ERR "Invalid appIdx. Only values within range 0x0001-0xfffe are vaild application indexes."
#define APP_ALLOC_ERR "Failed to allocate application key: application key storage full. An application key must be removed before another can be allocated."

static char app_idx_strings[APP_KEY_COUNT][7];

static int appkey_list(const struct shell *shell, size_t argc, char **argv)
{
        ARG_UNUSED(argc);
        ARG_UNUSED(argv);

        uint8_t i;
        uint8_t j;
        uint8_t idx;
        uint16_t lowest_idx;
        uint16_t last_lowest_idx;
        char app_key_str[33];
        struct bt_mesh_cdb_app_key *app_key_ptr;

        shell_info(shell, "Net Index : App Index : App Key");

        idx = 0;
        lowest_idx = 0xFFFF;
        last_lowest_idx = 0x0000;
        app_key_ptr = bt_mesh_cdb.app_keys;
        for (i = 0; i < APP_KEY_COUNT; i++) {
                for (j = 0; j < APP_KEY_COUNT; j++) {
                        if (app_key_ptr[j].net_idx != BT_MESH_KEY_UNUSED) {
                                if ((app_key_ptr[j].app_idx < lowest_idx)) {
                                        if ((i == 0) || (app_key_ptr[j].app_idx > last_lowest_idx)) {
                                                lowest_idx = app_key_ptr[j].app_idx;
                                                idx = j;
                                        }   
                                }
                        }
                }

                if (lowest_idx == BT_MESH_KEY_UNUSED) {
                        if (i == 0) {
                                shell_print(shell, "No Application Keys");
                        }

                        shell_print(shell, ""); 
                        return 0;
                }

                util_key2str(app_key_ptr[idx].keys[0].app_key, app_key_str);
                shell_print(shell, "  0x%04x  :   0x%04x  : %s", app_key_ptr[idx].net_idx,
                                app_key_ptr[idx].app_idx, app_key_str); 

                last_lowest_idx = lowest_idx;
                lowest_idx = 0xFFFF;
        }

        shell_print(shell, "");
        return 0;
}

static void get_dynamic_appkeys(size_t idx, struct shell_static_entry *entry)
{
        uint8_t i;
        uint8_t j;
        uint16_t lowest_idx;
        uint16_t last_lowest_idx;
        struct bt_mesh_cdb_app_key *app_key_ptr;

        /* If this is the first time through, create and sort a list of net indexes */
        lowest_idx = 0xFFFF;
        last_lowest_idx = 0x0000;
        app_key_ptr = bt_mesh_cdb.app_keys;
        if (idx == 0) {
                for (i = 0; i < APP_KEY_COUNT; i++)
                {
                        for (j = 0; j < APP_KEY_COUNT; j++) {
                                if ((app_key_ptr[j].net_idx != BT_MESH_KEY_UNUSED) &&
                                                (app_key_ptr[j].app_idx < lowest_idx)) {

                                        if ((app_key_ptr[j].app_idx > last_lowest_idx) || (i == 0)) {
                                                lowest_idx = app_key_ptr[j].app_idx;
                                        }
                                }
                        }
                        snprintf(app_idx_strings[i], sizeof(app_idx_strings[i]), "0x%04x", lowest_idx);
                        last_lowest_idx = lowest_idx;
                        lowest_idx = 0xFFFF;
                }
        }

        if (!strcmp(app_idx_strings[idx], "0xffff"))
        {
                entry->syntax = NULL;
                return;
        }

        entry->syntax = app_idx_strings[idx];
        entry->subcmd = NULL;
        entry->help = DYNAMIC_APPKEY_VIEW_HELP;
}

static int appkey_view(const struct shell *shell, size_t argc, char **argv)
{
        uint16_t app_idx;
        char app_key_str[KEY_STR_LEN];
        struct bt_mesh_cdb_app_key *app_key_ptr;

        if (!str_to_uint16(argv[0], &app_idx)) {
                shell_error(shell, "Invalid appIdx: %s. Must be 16-bit unsigned integer.", argv[0]);
                return -EINVAL;
        }

        app_key_ptr = bt_mesh_cdb_app_key_get(app_idx);
        if (app_key_ptr == NULL) {
                shell_error(shell, "%s: %s\n", argv[0], APP_GET_ERR);
                return -ENOEXEC;
        }
        util_key2str(app_key_ptr->keys[0].app_key, app_key_str);

        shell_info(shell,  "Net Index : App Index : App Key");
        shell_print(shell, "  0x%04x  :   0x%04x  : %s\n",
                        app_key_ptr->net_idx, app_idx, app_key_str);
        return 0;
}

static void get_dynamic_appkey_view(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_appkeys(idx, entry);
        entry->handler = appkey_view;
        entry->args.mandatory = 1;
        entry->args.optional = 0;
}

static int get_app_idx(const struct shell *shell, char **arg, uint16_t *app_idx, bool opt)
{
        char *app_idx_ptr;

        /* If the app_idx is an optional argument it must have an argument key infrnt of it */
        if (opt) {
                if (strcmp(arg[0], "-i")) {
                        shell_error(shell, "%s: %s\n", arg[0], ARG_KEY_ERR);
                        return -EINVAL;
                }
                app_idx_ptr = arg[1];
        } else {
                app_idx_ptr = arg[0];
        }

        if (!str_to_uint16(app_idx_ptr, app_idx)) {
                shell_error(shell, "%s: %s\n", app_idx_ptr, APP_IDX_ERR);
                return -EINVAL;
        }

        if (*app_idx == 0xFFFF) {
                shell_error(shell, "%s: %s\n", app_idx_ptr, APP_IDX_RANGE_ERR);
                return -EINVAL;
        }

        return 0;
}

static void get_lowest_app_idx(uint16_t *app_idx)
{
        /* Find lowest app index to use */
        *app_idx = 0x0000;
        while(bt_mesh_cdb_app_key_get(*app_idx) != NULL) {
                *app_idx = *app_idx + 1;
        }
}

static int create_app_key(const struct shell *shell, uint16_t net_idx, uint16_t app_idx, uint8_t app_key[KEY_LEN])
{
        uint8_t err;
        struct bt_mesh_cdb_app_key *app_key_ptr;

        /* Allocate new app key in CDB */
        app_key_ptr = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
        if (app_key_ptr == NULL) {
                shell_error(shell, "%s\n", APP_ALLOC_ERR);
                return -ENOEXEC;
        }

        /* Add application key and store in CDB */
        memcpy(app_key_ptr->keys[0].app_key, app_key, KEY_LEN);
        bt_mesh_cdb_app_key_store(app_key_ptr);

        err = bt_mesh_app_key_add(app_idx, net_idx, app_key);
        
        if (err) {
                bt_mesh_cdb_app_key_del(app_key_ptr, true); 
                /* CDB sets the net_idx to BT_MESH_ADDR_UNASSIGNED which is not the same
                 * as the default state. Set net_idx to default state manually so we can
                 * detect that the app_key storage location is free to use for new keys. */
                app_key_ptr->net_idx = BT_MESH_KEY_UNUSED;
                return -ENOEXEC;
        }

        return 0;
}

static int appkey_add(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        uint8_t app_key[KEY_LEN];

        /* Must have either 1 mandatory argument or 1 mandatory argument, 1
         * optional key and 1 optional argument */
        if (argc % 2 != 0) {
                shell_error(shell, "%s-i\n", ARG_NUM_ERR);
                return -EINVAL;
        }

        if (!str_to_uint16(argv[0], &net_idx)) {
                shell_error(shell, "%s: %s\n", argv[0], NET_KEY_ERR);
                return -EINVAL;
        }

        /* Make sure the key argument is valid */
        if (!util_key_valid(argv[1])) {
                shell_error(shell, "%s: %s\n", argv[1], APP_KEY_ERR);
                return -EINVAL;
        }
        util_str2key(argv[1], app_key);

        /* If the optional appIdx argument was provided... */
        if (argc > 2) {     /* User has specified app index to use */
                err = get_app_idx(shell, &argv[2], &app_idx, true);
                if (err) {
                        return err;
                }

                /* Make sure the requested app index is available */ 
                if (bt_mesh_cdb_app_key_get(app_idx) != NULL) {
                        shell_error(shell, "%s: %s\n", argv[3], APP_IDX_DUP_ERR);
                        return -EINVAL; 
                }
        } else {    /* No netIdx argument was provided, must use lowest available index */
                get_lowest_app_idx(&app_idx);
        }

        err = create_app_key(shell, net_idx, app_idx, app_key);
        if (err) {
                return err;
        }

        shell_info(shell, "Successfully added application key");
        shell_print(shell, "Key: %s", argv[1]);
        shell_print(shell, "Application Index: 0x%04x", app_idx);
        shell_print(shell, "Network Index: %s\n", argv[0]);
        return 0;
}

static void get_dynamic_appkey_add(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = appkey_add;
        entry->subcmd = NULL;
        entry->args.mandatory = 2;
        entry->args.optional = 2;
}

static int appkey_gen(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        uint8_t app_key[KEY_LEN];
        char app_key_str[KEY_STR_LEN];

        /* Must have either no arguments or 1 optional argument key + 1 optional argument */
        if (argc % 2  == 0) {
                shell_error(shell, "%s-i\n", ARG_NUM_ERR);
                return -EINVAL;
        }

        if (!str_to_uint16(argv[0], &net_idx)) {
                shell_error(shell, "%s: %s\n", argv[0], NET_KEY_ERR);
                return -EINVAL;
        }

        if (argc > 1) { /* User has specified app index to use */
                err = get_app_idx(shell, &argv[1], &app_idx, true);
                if (err) {
                        return err;
                }

                /* Make sure the requested app index is available */ 
                if (bt_mesh_cdb_app_key_get(app_idx) != NULL) {
                        shell_error(shell, "%s: %s\n", argv[1], APP_IDX_DUP_ERR);
                        return -EINVAL; 
                }
        } else { /* No appIdx argument was provided, must find lowest available index */
                get_lowest_app_idx(&app_idx);
        }

        /* Generate network key */
        bt_rand(app_key, KEY_LEN);
        err = create_app_key(shell, net_idx, app_idx, app_key);
        if (err) {
                return err;
        }

        util_key2str(app_key, app_key_str);
        shell_info(shell, "Successfully generated application key");
        shell_print(shell, "Application Key: %s", app_key_str);
        shell_print(shell, "Application Index: 0x%04x", app_idx);
        shell_print(shell, "Network Index: 0x%04x\n", net_idx);
        return 0;
}

static void get_dynamic_appkey_gen(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = appkey_gen;
        entry->subcmd = NULL;
        entry->args.mandatory = 1;
        entry->args.optional = 2;
}

static int appkey_rem(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        uint16_t net_idx;
        uint16_t app_idx;
        struct bt_mesh_cdb_app_key *app_key_ptr;

        err = get_app_idx(shell, argv, &app_idx, false);
        if (err) {
                return err;
        }

        app_key_ptr = bt_mesh_cdb_app_key_get(app_idx);
        if (app_key_ptr == NULL) {
                shell_error(shell, "%s: %s\n", argv[0], APP_GET_ERR);
                return -ENOEXEC;
        }

        net_idx = app_key_ptr->net_idx;
        bt_mesh_cdb_app_key_del(app_key_ptr, true); 
        /* CDB sets the net_idx to BT_MESH_ADDR_UNASSIGNED which is not the same
         * as the default state. Set net_idx to default state manually so we can
         * detect that the app_key storage location is free to use for new keys. */
        app_key_ptr->net_idx = BT_MESH_KEY_UNUSED;
        bt_mesh_app_key_del(app_idx, net_idx);
        return 0;
}

static void get_dynamic_appkey_rem(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_appkeys(idx, entry);
        entry->handler = appkey_rem;
        entry->args.mandatory = 1;
        entry->args.optional = 0;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_appkey_view, get_dynamic_appkey_view);
SHELL_DYNAMIC_CMD_CREATE(dynamic_appkey_add, get_dynamic_appkey_add);
SHELL_DYNAMIC_CMD_CREATE(dynamic_appkey_gen, get_dynamic_appkey_gen);
SHELL_DYNAMIC_CMD_CREATE(dynamic_appkey_rem, get_dynamic_appkey_rem);

#define APPKEY_HELP \
        "Application key database interface.\n"
#define APPKEY_LIST_HELP \
        "List all application index:key pairs stored on the gateway.\n" \
"USAGE\n" \
"appkey list\n"
#define APPKEY_VIEW_HELP \
        "View a stored application key by entering the associated application index.\n" \
"USAGE:\n" \
"appkey view <appIdx>\n" \
"- appIdx: The application index associated with the application key to be viewed.\n"
#define APPKEY_ADD_HELP \
        "Add a new application key to the gateway.\n" \
"USAGE\n" \
"appkey add <netIdx> <appKey> [-i appIdx]\n" \
"- netIdx: Unsigned 16-bit network index which the application key belongs to.\n" \
"- appKey: 128-bit application key represented by 32 hexadecimal characters.\n" \
"- appIdx: Unsigned 16-bit application index associated with the application key.\n"
#define APPKEY_GEN_HELP \
        "Generate a new application key.\n" \
"USAGE\n" \
"appkey generate <netIdx> [-i appIdx]\n" \
"- netIdx: Unsigned 16-bit network index which the application key belongs to.\n" \
"- appIdx: Unsigned 16-bit application index associated with the application key.\n"
#define APPKEY_REM_HELP \
        "Remove an application key associated with a specific application index.\n" \
"USAGE\n" \
"appkey remove <appIdx>\n" \
"- appIdx: Unsigned 16-bit application index associated with the application key.\n"

SHELL_STATIC_SUBCMD_SET_CREATE(appkey_subs,
                SHELL_CMD_ARG(list, NULL, APPKEY_LIST_HELP, appkey_list, 1, 0),
                SHELL_CMD(view, &dynamic_appkey_view, APPKEY_VIEW_HELP, NULL),
                SHELL_CMD(add, &dynamic_appkey_add, APPKEY_ADD_HELP, NULL),
                SHELL_CMD(generate, &dynamic_appkey_gen, APPKEY_GEN_HELP, NULL),
                SHELL_CMD(remove, &dynamic_appkey_rem, APPKEY_REM_HELP, NULL),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(appkey, &appkey_subs, APPKEY_HELP, NULL);


/******************************************************************************
 *  NODE CONFIGURATION COMMANDS
 *****************************************************************************/
/* Error Mesages */
#define STATUS_ERR \
        "Status Error:"
#define CMD_ERR \
        "Unrecognized command: "
#define NUM_ARGS_ERR \
        "Invalid number of arguments."

/* Argument help messages */
#define ARG_UUID_HELP \
        "128-bit UUID represented by 32 hexadecimal characters.\n"
#define ARG_ADDR_HELP \
        "Unsigned 16-bit address of target node.\n"
#define ARG_STATE_HELP \
        "ENABLE or DISABLE state of a feature. Either 0, 1, disable, enable, false, true.\n"
#define ARG_TTL_HELP \
        "Unsigned 8-bit value representing time-to-live in seconds.\n"
#define ARG_COUNT_HELP \
        "Retransmit count (not including first transmission). Value within range 0-7.\n"
#define ARG_INTERVAL_HELP \
        "Retransmit interval in milliseconds. Value within range 1-320 and a multiple of 10.\n"
#define ARG_NET_IDX_HELP \
        "Unsigned 16-bit network index of target subnet.\n"
#define ARG_APP_IDX_HELP \
        "Unsigned 16-bit application index of the target application key.\n"
#define ARG_ELEM_ADDR_HELP \
        "Unsigned 16-bit address of the target element within a node.\n"
#define ARG_MODEL_ID_HELP \
        "Unsigned 16-bit model ID of the target model.\n"
#define ARG_CID_HELP \
        "Unsigned 16-bit company identifier for vendor defined items.\n"
#define ARG_TEST_ID_HELP \
	"Unsigned 8-bit test ID.\n"
#define ARG_DIV_HELP \
	"Unsigned 8-bit devisor value.\n"
#define ARG_ATTN_HELP \
	"Unsigned 8-bit attention timer value in seconds.\n"
#define ARG_PUB_ADDR_HELP \
        "Unsigned 16-bit address for a model to publish to.\n"
#define ARG_PUB_APP_IDX_HELP \
        "Unsigned 16-bit application index for a model to publish with.\n"
#define ARG_CRED_FLAG_HELP \
        "Friend credential flag. Either 0, 1, disable, enable, false, true\n"
#define ARG_PERIOD_KEY_HELP \
        "Period type identifier. Either 100ms, 1s, 10s, 10m.\n"
#define ARG_PERIOD_HELP \
        "Publish period for model in mutliples of 'periodKey'. Value withint range 0-63.\n"
#define ARG_SUB_ADDR_HELP \
        "Unsigned 16-bit address for a model to subscribe to. Value within range 0xC000-0xFFFF\n"
#ifdef CONFIG_SHELL_MESH_MSG
#define ARG_MSG_ADDR_HELP \
        "Unsigned 16-bit mesh address which model messages can be published to\n"
#endif // CONFIG_SHELL_MESH_MSG
#define ARG_OPCODE_HELP \
        "Unsigned 32-bit mesh model opcode\n"
#define ARG_PAYLOAD_HELP \
        "Byte array in hexidecimal format\n"
#define ARG_TIMEOUT_HELP \
        "32-bit timeout value in milliseconds\n"
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
#define ARG_HB_DST_HELP \
	"Destination address of a node's heartbeat messages\n"
#define ARG_HB_PUB_COUNT_HELP \
	"Logarithmic heartbeat message send count. Decoded as (1 << (count - 1)).\n"
#define ARG_HB_PUB_PERIOD_HELP \
	"Logarithmic heartbeat publication transmit interval in seconds decoded as (1 << (period - 1)).\n"
#define ARG_HB_PUB_RELAY_STATE_HELP \
        "ENABLE or DISABLE relay heartbeat publications. Either 0, 1, disable, enable, false, true.\n"
#define ARG_HB_PUB_PROXY_STATE_HELP \
        "ENABLE or DISABLE proxy heartbeat publications. Either 0, 1, disable, enable, false, true.\n"
#define ARG_HB_PUB_FRIEND_STATE_HELP \
        "ENABLE or DISABLE friend heartbeat publications. Either 0, 1, disable, enable, false, true.\n"
#define ARG_HB_PUB_LPN_STATE_HELP \
        "ENABLE or DISABLE low-power-node heartbeat publications. Either 0, 1, disable, enable, false, true.\n"
#define ARG_HB_SUB_SRC_HELP \
	"Unsigned 16-bit source address to recieve heartbeat messages from.\n"
#define ARG_HB_SUB_DEST_HELP \
	"Unsigned 16-bit destination address to recieve heartbeat messages on.\n"
#define ARG_HB_SUB_PERIOD_HELP \
	"Logarithmic subscription period to keep listening for. Decoded as (1 << (period - 1)) seconds.\n"
#define ARG_HB_SUB_COUNT_HELP \
	"Logarithmic heartbeat message recieve count. Decoded as (1 << (count - 1)).\n"
#define ARG_HB_SUB_MIN_HELP \
	"Minimum hops in recieved messages, ie shortest path from source to destination.\n"
#define ARG_HB_SUB_MAX_HELP \
	"Maximum hops in recieved messages, ie longest path from source to destination.\n"
#endif // CONFIG_SHELL_MESH_HEARTBEAT

/* Command help messages */
#define BEACON_HELP \
        "Unprovisioned device beacon interface.\n"
#define BEACON_LIST_HELP \
        "List active unprovisioned device beacons.\n" \
        "USAGE:\n" \
        "beacon list\n"
#define BEACON_BLOCK_HELP \
        "Block unprovisioned device beacons with a specific UUID.\n" \
        "USAGE:\n" \
        "beacon block <UUID>\n" \
        " - UUID: " ARG_UUID_HELP
#define BEACON_UNBLOCK_HELP \
        "Unblock unprovisioned device beacons with a specific UUID.\n" \
        "USAGE:\n" \
        "beacon unblock <UUID>\n" \
        " - UUID: " ARG_UUID_HELP
#define NODE_HELP \
        "Node configuration interface.\n"
#define MODEL_HELP \
        "Model configuration interface.\n"
#ifdef CONFIG_SHELL_MESH_MSG
#define MSG_HELP \
        "Mesh model message interface.\n"
#endif // CONFIG_SHELL_MESH_MSG

#ifdef CONFIG_SHELL_MESH_HEALTH
#define HLTH_HELP \
        "Node health interface.\n"
#endif // CONFIG_SHELL_MESH_HEALTH
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
#define HB_HELP \
	"Node heartbeat interface.\n"
#endif // CONFIG_SHELL_MESH_HEARTBEAT
#define DYNAMIC_ADDR_HELP \
        "Valid Address"
#define DYNAMIC_SUB_ADDR_HELP \
        "Valid Mesh Message Address"
#define NODE_LIST_HELP \
        "List all nodes in the mesh network.\n" \
        "USAGE:\n" \
        "node list\n"
#define NODE_RESET_HELP \
        "Reset a node and remove it from the network.\n" \
        "USAGE:\n" \
        "node reset <addr>\n" \
        "- addr: " ARG_ADDR_HELP
#define NODE_DISC_HELP \
        "Discover a node's details including composition data, feature states, keys, and addresses.\n" \
        "USAGE:\n" \
        "node discover <addr>\n" \
        "-addr: " ARG_ADDR_HELP
#define NODE_COMP_HELP \
        "Get the composition data of a node.\n" \
        "USAGE:\n" \
        "node comp <addr>\n" \
        "- addr: " ARG_ADDR_HELP
#define NODE_BEACON_HELP \
        "GET or SET a node's network beacon state.\n" \
        "USAGE:\n" \
        "node beacon get <addr>\n" \
        "node beacon set <addr> <state>\n" \
        "- addr : " ARG_ADDR_HELP \
        "- state: " ARG_STATE_HELP
#define NODE_TTL_HELP \
        "GET or SET a node's time-to-live value.\n" \
        "USAGE:\n" \
        "node ttl get <addr>\n" \
        "node ttl set <addr> <ttl>\n" \
        "- addr : " ARG_ADDR_HELP \
        "- ttl: " ARG_TTL_HELP
#define NODE_FRIEND_HELP \
        "GET or SET a node's friend feature state.\n" \
        "USAGE:\n" \
        "node friend get <addr>\n" \
        "node friend set <addr> <state>\n" \
        "- addr : " ARG_ADDR_HELP \
        "- state: " ARG_STATE_HELP
#define NODE_PROXY_HELP \
        "GET or SET a node's GATT proxy feature state.\n" \
        "USAGE:\n" \
        "node proxy get <addr>\n" \
        "node proxy set <addr> <state>\n" \
        "- addr : " ARG_ADDR_HELP \
        "- state: " ARG_STATE_HELP
#define NODE_RELAY_HELP \
        "GET or SET a node's relay feature state.\n" \
        "USAGE:\n" \
        "node relay get <addr>\n" \
        "node relay set <addr> <state> <count> <interval>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- state   : " ARG_STATE_HELP \
        "- count   : " ARG_COUNT_HELP \
        "- interval: " ARG_INTERVAL_HELP
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
#define NODE_HB_SUB_HELP \
	"SET or GET a node's heartbeat subscribe parameters.\n" \
	"USAGE:\n" \
	"node hb-sub-set <addr> <src> <dest> <period>  <min> <max>\n" \
	"- addr  : " ARG_ADDR_HELP \
	"- src   : " ARG_HB_SUB_SRC_HELP \
	"- dest  : " ARG_HB_SUB_DEST_HELP \
	"- period: " ARG_HB_SUB_PERIOD_HELP \
	"- count : " ARG_HB_SUB_COUNT_HELP \
	"- min   : " ARG_HB_SUB_MIN_HELP \
	" -max   : " ARG_HB_SUB_MAX_HELP
#define NODE_HB_PUB_HELP \
	"SET or GET a node's heartbeat publish parameters.\n" \
	"USAGE:\n" \
	"hb-pub-set <netIdx> <addr> <dest> <count> <period> <ttl> <relay> <proxy> <friend> <lpn>\n" \
	"- netIdx: " ARG_NET_IDX_HELP \
	"- addr  : " ARG_ADDR_HELP \
	"- dest  : " ARG_HB_DST_HELP \
	"- count : " ARG_HB_PUB_COUNT_HELP \
	"- period: " ARG_HB_PUB_PERIOD_HELP \
	"- ttl   : " ARG_TTL_HELP \
	"- relay : " ARG_HB_PUB_RELAY_STATE_HELP \
	"- proxy : " ARG_HB_PUB_PROXY_STATE_HELP \
	"- friend: " ARG_HB_PUB_FRIEND_STATE_HELP \
	"- lpn   : " ARG_HB_PUB_LPN_STATE_HELP
#endif // CONFIG_SHELL_MESH_HEARTBEAT
#define NODE_SUBNET_HELP \
        "ADD subnets to a node or GET a node's current subnets.\n" \
        "USAGE:\n" \
        "node subnet add <addr> <netIdx>\n" \
        "node subnet get <addr>\n" \
        "node subnet delete <addr> <netIdx>\n" \
        "- addr  : " ARG_ADDR_HELP \
        "- netIdx: " ARG_NET_IDX_HELP
#define NODE_CONFIG_HELP \
        "SET or UNSET the configured flag of a node.\n" \
        "USAGE:\n" \
        "node configured set <addr>\n" \
        "node configured unset <addr>\n" \
        "- addr: " ARG_ADDR_HELP
#if defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
#define MODEL_APP_KEY_HELP \
        "BIND or GET application keys for a model.\n" \
        "USAGE\n" \
        "model appkey bind <addr> <appIdx> <elemAddr> <modelId>\n" \
        "model appkey unbind <addr> <appIdx> <elemAddr> <modelId>\n" \
        "model appkey get <addr> <elemAddr> <modelId>\n" \
        "model appkey bind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>\n" \
        "model appkey unbind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>\n" \
        "model appkey get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- appIdx  : " ARG_APP_IDX_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP \
        "- cid     : " ARG_CID_HELP
#define MODEL_PUB_HELP \
        "GET or SET a models publish parameters.\n" \
        "USAGE\n" \
        "model publish get <addr> <elemAddr> <modelId>\n" \
        "model publish set <addr> <elemAddr> <modelId> <pubAddr> <pubAppIdx> <credFlag> <ttl>" \
        " <periodKey> <period> <count> <interval>\n" \
        "model publish get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "model publish set-vnd <addr> <elemAddr> <modelId> <cid> <pubAddr> <pubAppIdx> <credFlag>" \
        " <ttl> <periodKey> <period> <count> <interval>\n" \
        "- addr     : " ARG_ADDR_HELP \
        "- elemAddr : " ARG_ELEM_ADDR_HELP \
        "- modelId  : " ARG_MODEL_ID_HELP \
        "- cid      : " ARG_CID_HELP \
        "- pubAddr  : " ARG_PUB_ADDR_HELP \
        "- pubAppIdx: " ARG_PUB_APP_IDX_HELP \
        "- credFlag : " ARG_CRED_FLAG_HELP \
        "- ttl      : " ARG_TTL_HELP \
        "- periodKey: " ARG_PERIOD_KEY_HELP \
        "- period   : " ARG_PERIOD_HELP \
        "- count    : " ARG_COUNT_HELP \
        "- interval : " ARG_INTERVAL_HELP
#define MODEL_SUB_HELP \
        "ADD, DELETE, OVERWRITE, or GET a model's subscribe address.\n" \
        "USAGE\n" \
        "model subscribe add <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe delete <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe overwrite <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe get <addr> <elemAddr> <modelId>\n" \
        "model subscribe add-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe delete-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe overwrite-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP \
        "- cid     : " ARG_CID_HELP \
        "- subAddr : " ARG_SUB_ADDR_HELP
#endif // defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
#if defined(CONFIG_SHELL_MESH_SIG) && !defined(CONFIG_SHELL_MESH_VND)
#define MODEL_APP_KEY_HELP \
        "BIND or GET application keys for a model.\n" \
        "USAGE\n" \
        "model appkey bind <addr> <appIdx> <elemAddr> <modelId>\n" \
        "model appkey unbind <addr> <appIdx> <elemAddr> <modelId>\n" \
        "model appkey get <addr> <elemAddr> <modelId>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- appIdx  : " ARG_APP_IDX_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP
#define MODEL_PUB_HELP \
        "GET or SET a models publish parameters.\n" \
        "USAGE\n" \
        "model publish get <addr> <elemAddr> <modelId>\n" \
        "model publish set <addr> <elemAddr> <modelId> <pubAddr> <pubAppIdx> <credFlag> <ttl>" \
        " <periodKey> <period> <count> <interval>\n" \
        "- addr     : " ARG_ADDR_HELP \
        "- elemAddr : " ARG_ELEM_ADDR_HELP \
        "- modelId  : " ARG_MODEL_ID_HELP \
        "- pubAddr  : " ARG_PUB_ADDR_HELP \
        "- pubAppIdx: " ARG_PUB_APP_IDX_HELP \
        "- credFlag : " ARG_CRED_FLAG_HELP \
        "- ttl      : " ARG_TTL_HELP \
        "- periodKey: " ARG_PERIOD_KEY_HELP \
        "- period   : " ARG_PERIOD_HELP \
        "- count    : " ARG_COUNT_HELP \
        "- interval : " ARG_INTERVAL_HELP
#define MODEL_SUB_HELP \
        "ADD, DELETE, OVERWRITE, or GET a model's subscribe address.\n" \
        "USAGE\n" \
        "model subscribe add <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe delete <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe overwrite <addr> <elemAddr> <modelId> <subAddr>\n" \
        "model subscribe get <addr> <elemAddr> <modelId>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP \
        "- subAddr : " ARG_SUB_ADDR_HELP
#endif // defined(CONFIG_SHELL_MESH_SIG) && !defined(CONFIG_SHELL_MESH_VND)
#if !defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
#define MODEL_APP_KEY_HELP \
        "BIND or GET application keys for a model.\n" \
        "USAGE\n" \
        "model appkey bind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>\n" \
        "model appkey unbind-vnd <addr> <appIdx> <elemAddr> <modelId> <cid>\n" \
        "model appkey get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- appIdx  : " ARG_APP_IDX_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP \
        "- cid     : " ARG_CID_HELP
#define MODEL_PUB_HELP \
        "GET or SET a models publish parameters.\n" \
        "USAGE\n" \
        "model publish get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "model publish set-vnd <addr> <elemAddr> <modelId> <cid> <pubAddr> <pubAppIdx> <credFlag>" \
        " <ttl> <periodKey> <period> <count> <interval>\n" \
        "- addr     : " ARG_ADDR_HELP \
        "- elemAddr : " ARG_ELEM_ADDR_HELP \
        "- modelId  : " ARG_MODEL_ID_HELP \
        "- cid      : " ARG_CID_HELP \
        "- pubAddr  : " ARG_PUB_ADDR_HELP \
        "- pubAppIdx: " ARG_PUB_APP_IDX_HELP \
        "- credFlag : " ARG_CRED_FLAG_HELP \
        "- ttl      : " ARG_TTL_HELP \
        "- periodKey: " ARG_PERIOD_KEY_HELP \
        "- period   : " ARG_PERIOD_HELP \
        "- count    : " ARG_COUNT_HELP \
        "- interval : " ARG_INTERVAL_HELP
#define MODEL_SUB_HELP \
        "ADD, DELETE, OVERWRITE, or GET a model's subscribe address.\n" \
        "USAGE\n" \
        "model subscribe add-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe delete-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe overwrite-vnd <addr> <elemAddr> <modelId> <cid> <subAddr>\n" \
        "model subscribe get-vnd <addr> <elemAddr> <modelId> <cid>\n" \
        "- addr    : " ARG_ADDR_HELP \
        "- elemAddr: " ARG_ELEM_ADDR_HELP \
        "- modelId : " ARG_MODEL_ID_HELP \
        "- cid     : " ARG_CID_HELP \
        "- subAddr : " ARG_SUB_ADDR_HELP
#endif // defined(CONFIG_SHELL_MESH_SIG) && !defined(CONFIG_SHELL_MESH_VND)
#ifdef CONFIG_SHELL_MESH_MSG
#define MSG_SUB_HELP \
        "Subscribe to a mesh address to view mesh messages.\n" \
        "USAGE:\n" \
        "message subscribe <addr>\n" \
        "- addr: " ARG_MSG_ADDR_HELP
#define MSG_UNSUB_HELP \
        "Unsubscribe from a mesh address to view mesh messages.\n" \
        "USAGE:\n" \
        "message unsubscribe <addr>\n" \
        "- addr: " ARG_MSG_ADDR_HELP
#define MSG_SEND_HELP \
        "Send a mesh model message.\n" \
        "USAGE:\n" \
        "message send <netIdx> <appIdx> <addr> <opcode> [payload]\n" \
        "- netIdx : " ARG_NET_IDX_HELP \
        "- appIdx : " ARG_APP_IDX_HELP \
        "- addr   : " ARG_ADDR_HELP \
        "- opcode : " ARG_OPCODE_HELP \
        "- payload: " ARG_PAYLOAD_HELP
#endif // CONFIG_SHELL_MESH_MSG

#ifdef CONFIG_SHELL_MESH_HEALTH
#define HLTH_FAULT_HELP \
        "GET, CLEAR, or TEST a node's fault status.\n" \
        "USAGE:\n" \
        "health fault get <addr> <appIdx> <cid>\n" \
        "health fault clear <addr> <appIdx> <cid>\n" \
        "health fault test <addr> <appIdx> <cid> <testId>\n" \
        "- addr  : " ARG_ADDR_HELP \
        "- appIdx: " ARG_APP_IDX_HELP \
        "- cid   : " ARG_CID_HELP \
	"- testId: " ARG_TEST_ID_HELP
#define HLTH_PERIOD_HELP \
        "GET or SET a node's health fast period divisor.\n" \
        "USAGE:\n" \
        "health period get <addr> <appIdx>\n" \
        "health period set <addr> <appIdx> <divisor>\n" \
        "- addr   : " ARG_ADDR_HELP \
        "- appIdx : " ARG_APP_IDX_HELP \
        "- divisor: " ARG_DIV_HELP
#define HLTH_ATTN_HELP \
        "GET or SET a node's attention timer.\n" \
        "USAGE:\n" \
        "health attention get <addr> <appIdx>\n" \
        "health attention set <addr> <appIdx> <attn>\n" \
        "- addr  : " ARG_ADDR_HELP \
        "- appIdx: " ARG_APP_IDX_HELP \
        "- attn : " ARG_ATTN_HELP
#define HLTH_TIMEOUT_HELP \
        "GET or SET the transmission timeout value for gateway's health client.\n" \
        "USAGE:\n" \
        "health timeout get\n" \
        "health timeout set <timeout>\n" \
        "- timeout: " ARG_TIMEOUT_HELP
#endif // CONFIG_SHELL_MESH_HEALTH

/* Dynamic argument help messages */
#define DYNAMIC_UNBLOCK_HELP \
        "Blocked UUID."
#define NODE_GET_HELP \
        "GET a node configuration item."
#define NODE_SET_HELP \
        "SET a node configuration item."
#define NODE_UNSET_HELP \
        "UNSET a node configuaration item."
#define NODE_ADD_HELP \
        "ADD a configuration item to a node."
#define NODE_CMD_DELETE_HELP \
        "DELETE a configuration item on a node."
#ifdef CONFIG_SHELL_MESH_SIG
#define MODEL_GET_HELP \
        "GET a SIG model configuration item."
#define MODEL_SET_HELP \
        "SET a SIG model configuration item."
#define MODEL_ADD_HELP \
        "ADD a SIG configuration item to a model."
#define MODEL_BIND_HELP \
        "BIND a configuration item to a SIG model."
#define MODEL_UNBIND_HELP \
        "UNBIND a configuration item from a SIG model."
#define MODEL_DEL_HELP \
        "Delete a configuration item on a SIG model."
#define MODEL_OVRW_HELP \
        "Overwrite a configuration item on a SIG model."
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
#define MODEL_GET_VND_HELP \
        "GET a VENDOR model configuration item."
#define MODEL_SET_VND_HELP \
        "SET a VENDOR model configuration item."
#define MODEL_ADD_VND_HELP \
        "ADD a VENDOR configuration item to a model."
#define MODEL_BIND_VND_HELP \
        "BIND a configuration item to a VENDOR model."
#define MODEL_UNBIND_VND_HELP \
        "UNBIND a configuration item from a VENDOR model."
#define MODEL_DEL_VND_HELP \
        "Delete a configuration item on a VENDOR model."
#define MODEL_OVRW_VND_HELP \
        "Overwrite a configuration item on a VENDOR model."
#endif // CONFIG_SHELL_MESH_VND

#ifdef CONFIG_SHELL_MESH_HEALTH
#define FAULT_GET_HELP \
	"Get the current fault status of a node."
#define FAULT_CLEAR_HELP \
	"Clear faults on a node."
#define FAULT_TEST_HELP \
	"Have a node run hrough its fault test."
#define PERIOD_GET_HELP \
	"Get a node's fast publish period devisor for when a fault is detected."
#define PERIOD_SET_HELP \
	"Set a node's fast publish period devisor for when a fault is detected."
#define ATTN_GET_HELP \
	"Get a node's attention timer value."
#define ATTN_SET_HELP \
	"Set a node's attention timer value."
#define TIMEOUT_GET_HELP \
	"Get the gateways health client model transmit timeout value."
#define TIMEOUT_SET_HELP \
	"Set the gateways health client model transmit timeout value."
#endif // CONFIG_SHELL_MESH_HEALTH

enum status_msg {
        STATUS_SUCCESS,
        STATUS_INVALID_ADDRESS,
        STATUS_INVALID_MODEL,
        STATUS_INVALID_APPKEY_INDEX,
        STATUS_INVALID_NETKEY_INDEX,
        STATUS_INSUFFICIENT_RESOURCES,
        STATUS_KEY_INDEX_ALREADY_STORED,
        STATUS_INVALID_PUBLISH_PARAMETERS,
        STATUS_NOT_A_SUBSCRIBE_MODEL,
        STATUS_STORAGE_FAILURE,
        STATUS_FEATURE_NOT_SUPPORTED,
        STATUS_CANNOT_UPDATE,
        STATUS_CANNOT_REMOVE,
        STATUS_CANNOT_BIND,
        STATUS_TEMPORARILY_UNABLE_TO_CHANGE_STATUS,
        STATUS_CANNOT_SET,
        STATUS_UNSPECIFIED_ERROR,
        STATUS_INVALID_BINDING
};

enum cmd_type {
        CMD_BEACON_LIST,
        CMD_BEACON_BLOCK,
        CMD_BEACON_UNBLOCK,
        CMD_NODE_LIST,
        CMD_NODE_RESET,
        CMD_NODE_DISC,
        CMD_NODE_COMP,
        CMD_NODE_BEACON_GET,
        CMD_NODE_BEACON_SET,
        CMD_NODE_TTL_GET,
        CMD_NODE_TTL_SET,
        CMD_NODE_FRIEND_GET,
        CMD_NODE_FRIEND_SET,
        CMD_NODE_PROXY_GET,
        CMD_NODE_PROXY_SET,
        CMD_NODE_RELAY_GET,
        CMD_NODE_RELAY_SET,
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
	CMD_NODE_HB_SUB_SET,
	CMD_NODE_HB_PUB_SET,
#endif // CONFIG_SHELL_MESH_HEARTBEAT
        CMD_NODE_SUBNET_ADD,
        CMD_NODE_SUBNET_GET,
        CMD_NODE_SUBNET_DEL,
        CMD_NODE_CONFIG_SET,
        CMD_NODE_CONFIG_UNSET,
#ifdef CONFIG_SHELL_MESH_SIG
        CMD_MODEL_APP_KEY_BIND,
        CMD_MODEL_APP_KEY_UNBIND,
        CMD_MODEL_APP_KEY_GET,
        CMD_MODEL_PUB_GET,
        CMD_MODEL_PUB_SET,
        CMD_MODEL_SUB_ADD,
        CMD_MODEL_SUB_DEL,
        CMD_MODEL_SUB_OVRW,
        CMD_MODEL_SUB_GET,
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
        CMD_MODEL_APP_KEY_BIND_VND,
        CMD_MODEL_APP_KEY_UNBIND_VND,
        CMD_MODEL_APP_KEY_GET_VND,
        CMD_MODEL_PUB_GET_VND,
        CMD_MODEL_PUB_SET_VND,
        CMD_MODEL_SUB_ADD_VND,
        CMD_MODEL_SUB_DEL_VND,
        CMD_MODEL_SUB_OVRW_VND,
        CMD_MODEL_SUB_GET_VND,
#endif // CONFIG_SHELL_MESH_VND
#ifdef CONFIG_SHELL_MESH_MSG
        CMD_MSG_SUB,
        CMD_MSG_UNSUB,
        CMD_MSG_SEND,
#endif // CONFIG_SHELL_MESH_MSG
#ifdef CONFIG_SHELL_MESH_HEALTH
        CMD_HLTH_FAULT_GET,
        CMD_HLTH_FAULT_CLR,
        CMD_HLTH_FAULT_TEST,
        CMD_HLTH_PERIOD_GET,
        CMD_HLTH_PERIOD_SET,
        CMD_HLTH_ATTN_GET,
        CMD_HLTH_ATTN_SET,
        CMD_HLTH_TIMEOUT_GET,
        CMD_HLTH_TIMEOUT_SET,
#endif
};

enum feature_status {
        FEATURE_DISABLED,
        FEATURE_ENABLED,
        FEATURE_NOT_SUPPORTED
};

enum period_key_type {
        PERIOD_KEY_100MS,
        PERIOD_KEY_1S,
        PERIOD_KEY_10S,
        PERIOD_KEY_10M
};

enum arg_type {
        ARG_UUID,
        ARG_ADDR,
        ARG_STATE,
        ARG_TTL,
        ARG_COUNT,
        ARG_INTERVAL,
        ARG_NET_IDX,
        ARG_APP_IDX,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
	ARG_TEST_ID,
	ARG_DIV,
	ARG_ATTN,
	ARG_TIMEOUT,
        ARG_PUB_ADDR,
        ARG_PUB_APP_IDX,
        ARG_CRED_FLAG,
        ARG_PERIOD_KEY,
        ARG_PERIOD,
        ARG_SUB_ADDR,
        ARG_OPCODE,
        ARG_PAYLOAD,
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
	ARG_HB_SRC,
	ARG_HB_DST,
	ARG_HB_PERIOD,
	ARG_HB_COUNT,
	ARG_HB_MIN,
	ARG_HB_MAX,
	ARG_HB_RELAY_STATE,
	ARG_HB_PROXY_STATE,
	ARG_HB_FRIEND_STATE,
	ARG_HB_LPN_STATE,
#endif // CONFIG_SHELL_MESH_HEARTBEAT
        ARG_TYPES_END
};

static const enum arg_type BEACON_LIST_ARGS[] = {
        ARG_TYPES_END
};
static const enum arg_type BEACON_BLOCK_ARGS[] = {
        ARG_UUID,
        ARG_TYPES_END
};
static const enum arg_type BEACON_UNBLOCK_ARGS[] = {
        ARG_UUID,
        ARG_TYPES_END
};
static const enum arg_type NODE_LIST_ARGS[] = {
        ARG_TYPES_END
};
static const enum arg_type NODE_RESET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_DISC_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_COMP_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_BEACON_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_BEACON_SET_ARGS[] = {
        ARG_ADDR,
        ARG_STATE,
        ARG_TYPES_END
};
static const enum arg_type NODE_TTL_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_TTL_SET_ARGS[] = {
        ARG_ADDR,
        ARG_TTL,
        ARG_TYPES_END
};
static const enum arg_type NODE_FRIEND_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_FRIEND_SET_ARGS[] = {
        ARG_ADDR,
        ARG_STATE,
        ARG_TYPES_END
};
static const enum arg_type NODE_PROXY_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_PROXY_SET_ARGS[] = {
        ARG_ADDR,
        ARG_STATE,
        ARG_TYPES_END
};
static const enum arg_type NODE_RELAY_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_RELAY_SET_ARGS[] = {
        ARG_ADDR,
        ARG_STATE,
        ARG_COUNT,
        ARG_INTERVAL,
        ARG_TYPES_END
};
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
static const enum arg_type NODE_HB_SUB_SET_ARGS[] = {
	ARG_ADDR,
	ARG_HB_SRC,
	ARG_HB_DST,
	ARG_HB_PERIOD,
	ARG_HB_COUNT,
	ARG_HB_MIN,
	ARG_HB_MAX,
	ARG_TYPES_END
};
static const enum arg_type NODE_HB_PUB_SET_ARGS[] = {
	ARG_NET_IDX,
	ARG_ADDR,
	ARG_HB_DST,
	ARG_HB_COUNT,
	ARG_HB_PERIOD,
	ARG_TTL,
	ARG_HB_RELAY_STATE,
	ARG_HB_PROXY_STATE,
	ARG_HB_FRIEND_STATE,
	ARG_HB_LPN_STATE,
	ARG_TYPES_END
};
#endif // CONFIG_SHELL_MESH_HEARTBEAT
static const enum arg_type NODE_SUBNET_ADD_ARGS[] = {
        ARG_ADDR,
        ARG_NET_IDX,
        ARG_TYPES_END
};
static const enum arg_type NODE_SUBNET_GET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_SUBNET_DEL_ARGS[] = {
        ARG_ADDR,
        ARG_NET_IDX,
        ARG_TYPES_END
};
static const enum arg_type NODE_CONFIG_SET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type NODE_CONFIG_UNSET_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
#ifdef CONFIG_SHELL_MESH_SIG
static const enum arg_type MODEL_APP_KEY_BIND_ARGS[] = {
        ARG_ADDR,
        ARG_APP_IDX,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_APP_KEY_UNBIND_ARGS[] = {
        ARG_ADDR,
        ARG_APP_IDX,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_APP_KEY_GET_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_PUB_GET_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_PUB_SET_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_PUB_ADDR,
        ARG_PUB_APP_IDX,
        ARG_CRED_FLAG,
        ARG_TTL,
        ARG_PERIOD_KEY,
        ARG_PERIOD,
        ARG_COUNT,
        ARG_INTERVAL,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_ADD_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_DEL_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_OVRW_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_GET_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_TYPES_END
};
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
static const enum arg_type MODEL_APP_KEY_BIND_VND_ARGS[] = {
        ARG_ADDR,
        ARG_APP_IDX,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_APP_KEY_UNBIND_VND_ARGS[] = {
        ARG_ADDR,
        ARG_APP_IDX,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_APP_KEY_GET_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_PUB_GET_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_TYPES_END
};
static const enum arg_type MODEL_PUB_SET_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_PUB_ADDR,
        ARG_PUB_APP_IDX,
        ARG_CRED_FLAG,
        ARG_TTL,
        ARG_PERIOD_KEY,
        ARG_PERIOD,
        ARG_COUNT,
        ARG_INTERVAL,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_ADD_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_DEL_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_OVRW_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_SUB_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MODEL_SUB_GET_VND_ARGS[] = {
        ARG_ADDR,
        ARG_ELEM_ADDR,
        ARG_MODEL_ID,
        ARG_CID,
        ARG_TYPES_END
};
#endif // CONFIG_SHELL_MESH_VND
#ifdef CONFIG_SHELL_MESH_MSG
static const enum arg_type MSG_SUB_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MSG_UNSUB_ARGS[] = {
        ARG_ADDR,
        ARG_TYPES_END
};
static const enum arg_type MSG_SEND_ARGS[] = {
        ARG_NET_IDX,
        ARG_APP_IDX,
        ARG_ADDR,
        ARG_OPCODE,
        ARG_PAYLOAD,
        ARG_TYPES_END
};
#endif // CONFIG_SHELL_MESH_MSG
#ifdef CONFIG_SHELL_MESH_HEALTH
static const enum arg_type HLTH_FAULT_GET_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_CID,
	ARG_TYPES_END
};
static const enum arg_type HLTH_FAULT_CLEAR_ARGS[] ={
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_CID,
	ARG_TYPES_END
};
static const enum arg_type HLTH_FAULT_TEST_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_CID,
	ARG_TEST_ID,
	ARG_TYPES_END
};
static const enum arg_type HLTH_PERIOD_GET_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_TYPES_END
};
static const enum arg_type HLTH_PERIOD_SET_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_DIV,
	ARG_TYPES_END
};
static const enum arg_type HLTH_ATTN_GET_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_TYPES_END
};
static const enum arg_type HLTH_ATTN_SET_ARGS[] = {
	ARG_ADDR,
	ARG_APP_IDX,
	ARG_ATTN,
	ARG_TYPES_END
};
static const enum arg_type HLTH_TIMEOUT_GET_ARGS[] = {
	ARG_TYPES_END
};
static const enum arg_type HLTH_TIMEOUT_SET_ARGS[] = {
	ARG_TIMEOUT,
	ARG_TYPES_END
};
#endif

static struct {
        enum cmd_type cmd;
        const enum arg_type *arg_list;
        struct {
                uint8_t uuid[32];
                uint16_t addr;
                bool state;
                uint8_t ttl;
                uint8_t transmit;
                uint8_t count;
                uint16_t interval;
                uint16_t app_idx;
                uint16_t net_idx;
                uint16_t elem_addr;
                uint16_t model_id;
                uint16_t cid;
		uint8_t test_id;
		uint8_t div;
		uint8_t attn;
		uint32_t timeout;
                uint16_t pub_addr;
                uint16_t pub_app_idx;
                bool cred_flag;
                enum period_key_type period_key;
                uint8_t period;
                uint16_t sub_addr;
                uint32_t opcode;
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		uint16_t hb_src;
		uint16_t hb_dst;
		uint8_t hb_period;
		uint8_t hb_count;
		uint8_t hb_min;
		uint8_t hb_max;
		bool hb_relay_state;
		bool hb_proxy_state;
		bool hb_friend_state;
		bool hb_lpn_state;
#endif // CONFIG_SHELL_MESH_HEARTBEAT
                uint8_t payload[MAX_PAYLOAD_LEN];
                uint8_t payload_len;
        } args;
} cmd;

static char node_addr_strings[NODE_COUNT][7];

static uint8_t node_print(struct bt_mesh_cdb_node *node, void *shell)
{
        char uuid_str[33];

        util_uuid2str(node->uuid, uuid_str);
        shell_print((const struct shell*)shell,
                        "    UUID: %s\n"
                        "    - Address      : 0x%04x\n"
                        "    - Net Index    : 0x%04x\n"
                        "    - Element Count: %d",
                        uuid_str, node->addr, node->net_idx, node->num_elem
                   );
        if (atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
                shell_print((const struct shell*)shell,
                                "    - Configured   : YES\n"
                           );
        } else {
                shell_print((const struct shell*)shell,
                                "    - Configured   : NO\n"
                           );
        }

        return BT_MESH_CDB_ITER_CONTINUE;
}

static void get_dynamic_addr(size_t idx, struct shell_static_entry *entry)
{
        uint8_t i;
        uint8_t j;
        uint16_t lowest_addr;
        uint16_t last_lowest_addr;
        struct bt_mesh_cdb_node *node_ptr;

        /* If this is the first time through, create and sort a list of net indexes */
        lowest_addr = 0xFFFF;
        last_lowest_addr = 0x0000;
        node_ptr = bt_mesh_cdb.nodes;
        if (idx == 0) {
                for (i = 0; i < NODE_COUNT; i++)
                {
                        for (j = 0; j < NODE_COUNT; j++) {
                                if ((node_ptr[j].addr < lowest_addr) &&
                                                (node_ptr[j].addr > last_lowest_addr)) {
                                        lowest_addr = node_ptr[j].addr;
                                }
                        }

                        snprintf(node_addr_strings[i], sizeof(node_addr_strings[i]),
                                        "0x%04x", lowest_addr);
                        last_lowest_addr = lowest_addr;
                        lowest_addr = 0xFFFF;
                }
        }

        if (!strcmp(node_addr_strings[idx], "0xffff"))
        {
                entry->syntax = NULL;
                return;
        }

        entry->syntax = node_addr_strings[idx];
        entry->help = DYNAMIC_ADDR_HELP;
}

#ifdef CONFIG_SHELL_MESH_MSG
static char addr_strings[CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN][7];

static void get_dynamic_sub_addr(size_t idx, struct shell_static_entry *entry)
{
        int i;
        int j;
        uint16_t lowest_addr;
        uint16_t last_lowest_addr;
        const uint16_t *addr_ptr;

        if (idx == 0) {
                last_lowest_addr = 0x0000;
                addr_ptr = btmesh_get_subscribe_list(BTMESH_SUB_TYPE_CLI);

                if (addr_ptr == NULL) {
                        entry->syntax = NULL;
                        return;
                }

                for (i = 0; i < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; i++) {
                        lowest_addr = 0xFFFF;
                        
                        for (j = 0; j < CONFIG_BT_MESH_GATEWAY_SUB_LIST_LEN; j++) {
                                if ((addr_ptr[j] < lowest_addr) && (addr_ptr[j] > last_lowest_addr)) {
                                        lowest_addr = addr_ptr[j];
                                }
                        }
                        
                        snprintf(addr_strings[i], sizeof(addr_strings[i]), "0x%04x", lowest_addr);
                        last_lowest_addr = lowest_addr;
                }
        }

        if (!strcmp(addr_strings[idx], "0xffff")) {
                entry->syntax = NULL;
                return;
        }

        entry->syntax = addr_strings[idx];
        entry->help = DYNAMIC_SUB_ADDR_HELP;
}
#endif // CONFIG_SHELL_MESH_MSG

static void print_feature_status(const struct shell *shell, uint8_t status)
{
        switch (status) {
                case FEATURE_DISABLED:
                        shell_print(shell, "  - DISABLED");
                        break;

                case FEATURE_ENABLED:
                        shell_print(shell, "  - ENABLED");
                        break;

                case FEATURE_NOT_SUPPORTED:
                        shell_print(shell, "  - NOT SUPPORTED");
                        break;

                default:
                        shell_error(shell, "  - UNKOWN: 0x%02x", status);
        }
}

static int cmd_parse_cmd(const struct shell *shell, char **argv)
{
        if (!strcmp(argv[-1], "beacon")) {
                if (!strcmp(argv[0], "list")) {
                        cmd.cmd = CMD_BEACON_LIST;
                        cmd.arg_list = BEACON_LIST_ARGS;
                } else if (!strcmp(argv[0], "block")) {
                        cmd.cmd = CMD_BEACON_BLOCK;
                        cmd.arg_list = BEACON_BLOCK_ARGS;
                } else if (!strcmp(argv[0], "unblock")) {
                        cmd.cmd = CMD_BEACON_UNBLOCK;
                        cmd.arg_list = BEACON_UNBLOCK_ARGS;
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                }
        }
        else if (!strcmp(argv[-1], "node")) {
                if (!strcmp(argv[0], "list")) {
                        cmd.cmd = CMD_NODE_LIST;
                        cmd.arg_list = NODE_LIST_ARGS;
                } else if (!strcmp(argv[0], "reset")) {
                        cmd.cmd = CMD_NODE_RESET;
                        cmd.arg_list = NODE_RESET_ARGS;
                } else if (!strcmp(argv[0], "discover")) {
                        cmd.cmd = CMD_NODE_DISC;
                        cmd.arg_list = NODE_DISC_ARGS;
                } else if (!strcmp(argv[0], "comp")) {
                        cmd.cmd = CMD_NODE_COMP;
                        cmd.arg_list = NODE_COMP_ARGS;
                } else if (!strcmp(argv[0], "beacon")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_BEACON_GET;
                                cmd.arg_list = NODE_BEACON_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_BEACON_SET;
                                cmd.arg_list = NODE_BEACON_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "ttl")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_TTL_GET;
                                cmd.arg_list = NODE_TTL_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_TTL_SET;
                                cmd.arg_list = NODE_TTL_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "friend")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_FRIEND_GET;
                                cmd.arg_list = NODE_FRIEND_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_FRIEND_SET;
                                cmd.arg_list = NODE_FRIEND_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "proxy")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_PROXY_GET;
                                cmd.arg_list = NODE_PROXY_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_PROXY_SET;
                                cmd.arg_list = NODE_PROXY_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "relay")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_RELAY_GET;
                                cmd.arg_list = NODE_RELAY_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_RELAY_SET;
                                cmd.arg_list = NODE_RELAY_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		} else if (!strcmp(argv[0], "hb-sub-set")) {
			cmd.cmd = CMD_NODE_HB_SUB_SET;
			cmd.arg_list = NODE_HB_SUB_SET_ARGS;
		} else if (!strcmp(argv[0], "hb-pub-set")) {
			cmd.cmd = CMD_NODE_HB_PUB_SET;
			cmd.arg_list = NODE_HB_PUB_SET_ARGS;
#endif // CONFIG_SHELL_MESH_HEARTBEAT
                } else if (!strcmp(argv[0], "subnet")) {
                        if (!strcmp(argv[1], "add")) {
                                cmd.cmd = CMD_NODE_SUBNET_ADD;
                                cmd.arg_list = NODE_SUBNET_ADD_ARGS;
                        } else if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_NODE_SUBNET_GET;
                                cmd.arg_list = NODE_SUBNET_GET_ARGS;
                        } else if (!strcmp(argv[1], "delete")) {
                                cmd.cmd = CMD_NODE_SUBNET_DEL;
                                cmd.arg_list = NODE_SUBNET_DEL_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "configured")) {
                        if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_NODE_CONFIG_SET;
                                cmd.arg_list = NODE_CONFIG_SET_ARGS;
                        } else if (!strcmp(argv[1], "unset")) {
                                cmd.cmd = CMD_NODE_CONFIG_UNSET;
                                cmd.arg_list = NODE_CONFIG_UNSET_ARGS;
                        } else {
                                shell_error(shell, "%S%S\n", CMD_ERR, argv[1]);
                        }
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                        return -EINVAL;
                }
#if defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
        } else if (!strcmp(argv[-1], "model")) {
                if (!strcmp(argv[0], "appkey")) {
                        if (!strcmp(argv[1], "bind")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_BIND;
                                cmd.arg_list = MODEL_APP_KEY_BIND_ARGS;
                        } else if (!strcmp(argv[1], "unbind")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_UNBIND;
                                cmd.arg_list = MODEL_APP_KEY_UNBIND_ARGS;
                        } else if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_GET;
                                cmd.arg_list = MODEL_APP_KEY_GET_ARGS;
			} else if (!strcmp(argv[1], "bind-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_BIND_VND;
                                cmd.arg_list = MODEL_APP_KEY_BIND_VND_ARGS;
                        } else if (!strcmp(argv[1], "unbind-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_UNBIND_VND;
                                cmd.arg_list = MODEL_APP_KEY_UNBIND_VND_ARGS;
                        } else if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_GET_VND;
                                cmd.arg_list = MODEL_APP_KEY_GET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "publish")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_PUB_GET;
                                cmd.arg_list = MODEL_PUB_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_MODEL_PUB_SET;
                                cmd.arg_list = MODEL_PUB_SET_ARGS;
			} else if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_PUB_GET_VND;
                                cmd.arg_list = MODEL_PUB_GET_VND_ARGS;
                        } else if (!strcmp(argv[1], "set-vnd")) {
                                cmd.cmd = CMD_MODEL_PUB_SET_VND;
                                cmd.arg_list = MODEL_PUB_SET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else if (!strcmp(argv[0], "subscribe")) {
                        if (!strcmp(argv[1], "add")) {
                                cmd.cmd = CMD_MODEL_SUB_ADD;
                                cmd.arg_list = MODEL_SUB_ADD_ARGS;
                        } else if (!strcmp(argv[1], "delete")) {
                                cmd.cmd = CMD_MODEL_SUB_DEL;
                                cmd.arg_list = MODEL_SUB_DEL_ARGS;
                        } else if (!strcmp(argv[1], "overwrite")) {
                                cmd.cmd = CMD_MODEL_SUB_OVRW;
                                cmd.arg_list = MODEL_SUB_OVRW_ARGS;
                        } else if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_SUB_GET;
                                cmd.arg_list = MODEL_SUB_GET_ARGS;
			} else if (!strcmp(argv[1], "add-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_ADD_VND;
                                cmd.arg_list = MODEL_SUB_ADD_VND_ARGS;
                        } else if (!strcmp(argv[1], "delete-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_DEL_VND;
                                cmd.arg_list = MODEL_SUB_DEL_VND_ARGS;
                        } else if (!strcmp(argv[1], "overwrite-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_OVRW_VND;
                                cmd.arg_list = MODEL_SUB_OVRW_VND_ARGS;
                        } else if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_GET_VND;
                                cmd.arg_list = MODEL_SUB_GET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                        return -EINVAL;
                }
#endif // defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
#if defined(CONFIG_SHELL_MESH_SIG) && !defined(CONFIG_SHELL_MESH_VND)
        } else if (!strcmp(argv[-1], "model")) {
                if (!strcmp(argv[0], "appkey")) {
                        if (!strcmp(argv[1], "bind")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_BIND;
                                cmd.arg_list = MODEL_APP_KEY_BIND_ARGS;
                        } else if (!strcmp(argv[1], "unbind")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_UNBIND;
                                cmd.arg_list = MODEL_APP_KEY_UNBIND_ARGS;
                        } else if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_GET;
                                cmd.arg_list = MODEL_APP_KEY_GET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "publish")) {
                        if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_PUB_GET;
                                cmd.arg_list = MODEL_PUB_GET_ARGS;
                        } else if (!strcmp(argv[1], "set")) {
                                cmd.cmd = CMD_MODEL_PUB_SET;
                                cmd.arg_list = MODEL_PUB_SET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else if (!strcmp(argv[0], "subscribe")) {
                        if (!strcmp(argv[1], "add")) {
                                cmd.cmd = CMD_MODEL_SUB_ADD;
                                cmd.arg_list = MODEL_SUB_ADD_ARGS;
                        } else if (!strcmp(argv[1], "delete")) {
                                cmd.cmd = CMD_MODEL_SUB_DEL;
                                cmd.arg_list = MODEL_SUB_DEL_ARGS;
                        } else if (!strcmp(argv[1], "overwrite")) {
                                cmd.cmd = CMD_MODEL_SUB_OVRW;
                                cmd.arg_list = MODEL_SUB_OVRW_ARGS;
                        } else if (!strcmp(argv[1], "get")) {
                                cmd.cmd = CMD_MODEL_SUB_GET;
                                cmd.arg_list = MODEL_SUB_GET_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                        return -EINVAL;
                }
#endif // defined(CONFIG_SHELL_MESH_SIG) && !defined(CONFIG_SHELL_MESH_VND)
#if !defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
        } else if (!strcmp(argv[-1], "model")) {
                if (!strcmp(argv[0], "appkey")) {
                        if (!strcmp(argv[1], "bind-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_BIND_VND;
                                cmd.arg_list = MODEL_APP_KEY_BIND_VND_ARGS;
                        } else if (!strcmp(argv[1], "unbind-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_UNBIND_VND;
                                cmd.arg_list = MODEL_APP_KEY_UNBIND_VND_ARGS;
                        } else if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_APP_KEY_GET_VND;
                                cmd.arg_list = MODEL_APP_KEY_GET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                                return -EINVAL;
                        }
                } else if (!strcmp(argv[0], "publish")) {
                        if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_PUB_GET_VND;
                                cmd.arg_list = MODEL_PUB_GET_VND_ARGS;
                        } else if (!strcmp(argv[1], "set-vnd")) {
                                cmd.cmd = CMD_MODEL_PUB_SET_VND;
                                cmd.arg_list = MODEL_PUB_SET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else if (!strcmp(argv[0], "subscribe")) {
                        if (!strcmp(argv[1], "add-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_ADD_VND;
                                cmd.arg_list = MODEL_SUB_ADD_VND_ARGS;
                        } else if (!strcmp(argv[1], "delete-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_DEL_VND;
                                cmd.arg_list = MODEL_SUB_DEL_VND_ARGS;
                        } else if (!strcmp(argv[1], "overwrite-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_OVRW_VND;
                                cmd.arg_list = MODEL_SUB_OVRW_VND_ARGS;
                        } else if (!strcmp(argv[1], "get-vnd")) {
                                cmd.cmd = CMD_MODEL_SUB_GET_VND;
                                cmd.arg_list = MODEL_SUB_GET_VND_ARGS;
                        } else {
                                shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
                        }
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                        return -EINVAL;
                }
#endif // !defined(CONFIG_SHELL_MESH_SIG) && defined(CONFIG_SHELL_MESH_VND)
#ifdef CONFIG_SHELL_MESH_MSG
        } else if (!strcmp(argv[-1], "message")) {
                if (!strcmp(argv[0], "subscribe")) {
                        cmd.cmd = CMD_MSG_SUB;
                        cmd.arg_list = MSG_SUB_ARGS;
                } else if (!strcmp(argv[0], "unsubscribe")) {
                        cmd.cmd = CMD_MSG_UNSUB;
                        cmd.arg_list = MSG_UNSUB_ARGS;
                } else if (!strcmp(argv[0], "send")) {
                        cmd.cmd = CMD_MSG_SEND;
                        cmd.arg_list = MSG_SEND_ARGS;
                } else {
                        shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
                        return -EINVAL;
                }
#endif // CONFIG_SHELL_MESH_MSG
#ifdef CONFIG_SHELL_MESH_HEALTH
	} else if (!strcmp(argv[-1], "health")) {
		if (!strcmp(argv[0], "fault")) {
			if (!strcmp(argv[1], "get")) {
				cmd.cmd = CMD_HLTH_FAULT_GET;
				cmd.arg_list = HLTH_FAULT_GET_ARGS;
			} else if (!strcmp(argv[1], "clear")) {
				cmd.cmd = CMD_HLTH_FAULT_CLR;
				cmd.arg_list = HLTH_FAULT_CLEAR_ARGS;
			} else if (!strcmp(argv[1], "test")) {
				cmd.cmd = CMD_HLTH_FAULT_TEST;
				cmd.arg_list = HLTH_FAULT_TEST_ARGS;
			} else {
				shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
				return -EINVAL;
			}
		} else if (!strcmp(argv[0], "period")) {
			if (!strcmp(argv[1], "get")) {
				cmd.cmd = CMD_HLTH_PERIOD_GET;
				cmd.arg_list = HLTH_PERIOD_GET_ARGS;
			} else if (!strcmp(argv[1], "set")) {
				cmd.cmd = CMD_HLTH_PERIOD_SET;
				cmd.arg_list = HLTH_PERIOD_SET_ARGS;
			} else {
				shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
				return -EINVAL;
			}
		} else if (!strcmp(argv[0], "attention")) {
			if (!strcmp(argv[1], "get")) {
				cmd.cmd = CMD_HLTH_ATTN_GET;
				cmd.arg_list = HLTH_ATTN_GET_ARGS;
			} else if (!strcmp(argv[1], "set")) {
				cmd.cmd = CMD_HLTH_ATTN_SET;
				cmd.arg_list = HLTH_ATTN_SET_ARGS;
			} else {
				shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
				return -EINVAL;
			}
		} else if (!strcmp(argv[0], "timeout")) {
			if (!strcmp(argv[1], "get")) {
				cmd.cmd = CMD_HLTH_TIMEOUT_GET;
				cmd.arg_list = HLTH_TIMEOUT_GET_ARGS;
			} else if (!strcmp(argv[1], "set")) {
				cmd.cmd = CMD_HLTH_TIMEOUT_SET;
				cmd.arg_list = HLTH_TIMEOUT_SET_ARGS;
			} else {
				shell_error(shell, "%s%s\n", CMD_ERR, argv[1]);
				return -EINVAL;
			}
		} else {
			shell_error(shell, "%s%s\n", CMD_ERR, argv[0]);
			return -EINVAL;
		}
#endif // CONFIG_SHELL_MESH_HEALTH
        } else {
                shell_error(shell, "%s%s\n", CMD_ERR, argv[-1]);
                return -EINVAL;
        }

        return 0;
}

static int cmd_parse_args(const struct shell *shell, size_t argc, char **argv)
{
        uint8_t i;
        uint8_t j;
        const enum arg_type *arg;

        arg = cmd.arg_list;

        /* Set the start of arguments within argv */
        switch (cmd.cmd) {
                case CMD_BEACON_LIST:
                case CMD_BEACON_BLOCK:
                case CMD_BEACON_UNBLOCK:
                case CMD_NODE_LIST:
                case CMD_NODE_RESET:
                case CMD_NODE_DISC:
                case CMD_NODE_COMP:
#ifdef CONFIG_SHELL_MESH_MSG
                case CMD_MSG_SUB:
                case CMD_MSG_UNSUB:
                case CMD_MSG_SEND:
#endif // CONFIG_SHELL_MESH_MSG
                        i = 1;
                        break;
                case CMD_NODE_BEACON_GET:
                case CMD_NODE_BEACON_SET:
                case CMD_NODE_TTL_GET:
                case CMD_NODE_TTL_SET:
                case CMD_NODE_FRIEND_GET:
                case CMD_NODE_FRIEND_SET:
                case CMD_NODE_PROXY_GET:
                case CMD_NODE_PROXY_SET:
                case CMD_NODE_RELAY_GET:
                case CMD_NODE_RELAY_SET:
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		case CMD_NODE_HB_SUB_SET:
		case CMD_NODE_HB_PUB_SET:
#endif // CONFIG_SHELL_MESH_HEARTBEAT
                case CMD_NODE_SUBNET_ADD:
                case CMD_NODE_SUBNET_GET:
                case CMD_NODE_SUBNET_DEL:
                case CMD_NODE_CONFIG_SET:
                case CMD_NODE_CONFIG_UNSET:
#ifdef CONFIG_SHELL_MESH_SIG
                case CMD_MODEL_APP_KEY_BIND:
                case CMD_MODEL_APP_KEY_UNBIND:
                case CMD_MODEL_APP_KEY_GET:
                case CMD_MODEL_PUB_GET:
                case CMD_MODEL_PUB_SET:
                case CMD_MODEL_SUB_ADD:
                case CMD_MODEL_SUB_DEL:
                case CMD_MODEL_SUB_OVRW:
                case CMD_MODEL_SUB_GET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
                case CMD_MODEL_APP_KEY_BIND_VND:
                case CMD_MODEL_APP_KEY_UNBIND_VND:
                case CMD_MODEL_APP_KEY_GET_VND:
                case CMD_MODEL_PUB_GET_VND:
                case CMD_MODEL_PUB_SET_VND:
                case CMD_MODEL_SUB_ADD_VND:
                case CMD_MODEL_SUB_DEL_VND:
		case CMD_MODEL_SUB_OVRW_VND:
                case CMD_MODEL_SUB_GET_VND:
#endif // CONFIG_SHELL_MESH_VND
#ifdef CONFIG_SHELL_MESH_HEALTH
		case CMD_HLTH_FAULT_GET:
		case CMD_HLTH_FAULT_CLR:
		case CMD_HLTH_FAULT_TEST:
		case CMD_HLTH_PERIOD_GET:
		case CMD_HLTH_PERIOD_SET:
		case CMD_HLTH_ATTN_GET:
		case CMD_HLTH_ATTN_SET:
		case CMD_HLTH_TIMEOUT_GET:
		case CMD_HLTH_TIMEOUT_SET:
#endif // CONFIG_SHELL_MESH_HEALTH
                        i = 2;
                        break;
                default:
                        /* Should be imposible */
                        i = UINT8_MAX;
                        break;
        }

        for (j = 0, arg = cmd.arg_list; arg[j] != ARG_TYPES_END; j++)
        {
                if (i+j >= argc) {
                        /* Paylaod arg for sending messages can be optional */
                        if (arg[j] == ARG_PAYLOAD) {
                                cmd.args.payload_len = 0;
                                return 0;
                        }

                        shell_error(shell, "%s Expected at least %d, Recieved %d\n",
                                        NUM_ARGS_ERR, i+j+1, argc);
                        return -EINVAL;
                }

                switch (arg[j]) {
                case ARG_UUID:
                        if (!util_uuid_valid(argv[i+j])) {
                                shell_error(shell, "Invalid UUID: %s. %s\n", argv[i+j],
                                                ARG_UUID_HELP);
                                return -EINVAL;
                        }

                        util_str2uuid(argv[i+j], cmd.args.uuid);
                        break;

                case ARG_ADDR:
                        if (!str_to_uint16(argv[i+j], &cmd.args.addr)) {
                                shell_error(shell, "Invalid addr: %s. %s\n", argv[i+j],
                                                ARG_ADDR_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_STATE:
                        if (!str_to_state(argv[i+j], &cmd.args.state)) {
                                shell_error(shell, "Invalid state: %s. %s\n", argv[i+j],
                                                ARG_STATE_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_TTL:
                        if (!str_to_uint8(argv[i+j], &cmd.args.ttl)) {
                                shell_error(shell, "Invalid ttl: %s. %s\n", argv[i+j],
                                                ARG_TTL_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_COUNT:
                        if (!str_to_uint8(argv[i+j], &cmd.args.count)) {
                                shell_error(shell, "Invalid count %s. %s\n", argv[i+j],
                                                ARG_COUNT_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_INTERVAL:
                        if (!str_to_uint16(argv[i+j], &cmd.args.interval)) {
                                shell_error(shell, "Invalid interval: %s. %s\n", argv[i+j],
                                                ARG_INTERVAL_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_NET_IDX:
                        if (!str_to_uint16(argv[i+j], &cmd.args.net_idx)) {
                                shell_error(shell, "Invalid netIdx: %s. %s\n", argv[i+j],
                                                ARG_NET_IDX_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_APP_IDX:
                        if (!str_to_uint16(argv[i+j], &cmd.args.app_idx)) {
                                shell_error(shell, "Invalid appIdx: %s. %s\n", argv[i+j],
                                                ARG_APP_IDX_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_ELEM_ADDR:
                        if (!str_to_uint16(argv[i+j], &cmd.args.elem_addr)) {
                                shell_error(shell, "Invalid elemAddr: %s. %s\n", argv[i+j],
                                                ARG_ELEM_ADDR_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_MODEL_ID:
                        if (!str_to_uint16(argv[i+j], &cmd.args.model_id)) {
                                shell_error(shell, "Invalid modeId: %s. %s\n", argv[i+j],
                                                ARG_MODEL_ID_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_CID:
                        if (!str_to_uint16(argv[i+j], &cmd.args.cid)) {
                                shell_error(shell, "Invalid cid: %s. %s\n", argv[i+j],
                                                ARG_CID_HELP);
                                return -EINVAL;
                        }

                        break;

		case ARG_TEST_ID:
			if (!str_to_uint8(argv[i+j], &cmd.args.test_id)) {
				shell_error(shell, "Invalid testId: %s. %s\n", argv[i+j],
						ARG_TEST_ID_HELP);
				return -EINVAL;
			}

			break;

		case ARG_DIV:
			if (!str_to_uint8(argv[i+j], &cmd.args.div)) {
				shell_error(shell, "Invalid divisor: %s. %s\n", argv[i+j],
						ARG_DIV_HELP);
				return -EINVAL;
			}

			break;

		case ARG_ATTN:
			if (!str_to_uint8(argv[i+j], &cmd.args.attn)) {
				shell_error(shell, "Invalid attn: %s. %s\n", argv[i+j],
						ARG_ATTN_HELP);
				return -EINVAL;
			}

			break;

		case ARG_TIMEOUT:
			if (!str_to_uint32(argv[i+j], (uint32_t*)&cmd.args.timeout)) {
				shell_error(shell, "Invalid timeout: %s. %s\n", argv[i+j],
						ARG_TIMEOUT_HELP);
				return -EINVAL;
			}

			break;

                case ARG_PUB_ADDR:
                        if (!str_to_uint16(argv[i+j], &cmd.args.pub_addr)) {
                                shell_error(shell, "Invalid pubAddr: %s. %s\n", argv[i+j],
                                                ARG_PUB_ADDR_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_PUB_APP_IDX:
                        if (!str_to_uint16(argv[i+j], &cmd.args.pub_app_idx)) {
                                shell_error(shell, "Invalid pubAppIdx: %s. %s\n", argv[i+j],
                                                ARG_APP_IDX_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_CRED_FLAG:
                        if (!str_to_state(argv[i+j], &cmd.args.cred_flag)) {
                                shell_error(shell, "Invalid credFlag: %s. %s\n", argv[i+j],
                                                ARG_CRED_FLAG_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_PERIOD_KEY:
                        if (!strcmp(argv[i+j], "100ms")) {
                                cmd.args.period_key = PERIOD_KEY_100MS;
                        } else if (!strcmp(argv[i+j], "1s")) {
                                cmd.args.period_key = PERIOD_KEY_1S;
                        } else if (!strcmp(argv[i+j], "10s")) {
                                cmd.args.period_key = PERIOD_KEY_10S;
                        } else if (!strcmp(argv[i+j], "10m")) {
                                cmd.args.period_key = PERIOD_KEY_10M;
                        } else {
                                shell_error(shell, "Invalid periodKey: %s. %s\n", argv[i+j],
                                                ARG_PERIOD_KEY_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_PERIOD:
                        if (!str_to_uint8(argv[i+j], &cmd.args.period)) {
                                shell_error(shell, "Invalid period: %s: %s\n", argv[i+j],
                                                ARG_PERIOD_HELP);
                                return -EINVAL;
                        }

                        if (cmd.args.period > 63) {
                                shell_error(shell, "Invalid period: %s. %s\n", argv[i+j],
                                                ARG_PERIOD_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_SUB_ADDR:
                        if (!str_to_uint16(argv[i+j], &cmd.args.sub_addr)) {
                                shell_error(shell, "Invalid subAddr: %s. %s\n", argv[i+j],
                                                ARG_SUB_ADDR_HELP);
                                return -EINVAL;
                        }

                        if (cmd.args.sub_addr < 0xC000) {
                                shell_error(shell, "Invalid subAddr: %s. %s\n", argv[i+j],
                                                ARG_SUB_ADDR_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_OPCODE:
                        if (!str_to_uint32(argv[i+j], &cmd.args.opcode)) {
                                shell_error(shell, "Invalid opcode: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

                        break;

                case ARG_PAYLOAD:
                        {
                                int len = 0;
                                int index;
                                char byte[5];

                                byte[0] = '0';
                                byte[1] = 'x';
                                byte[4] = '\0';

                                while (argv[i+j][len] != '\0') {
                                        len++;
                                        
                                        if (len > MAX_PAYLOAD_LEN) {
                                                shell_error(shell,
                                                                "Invalid payload: %s. %s\n",
                                                                argv[i+j], ARG_PAYLOAD_HELP);
                                                return -EINVAL;
                                        }
                                }

                                if (len % 2 == 0) {
                                       index = len / 2;
                                } else {
                                       index = (len / 2) + 1;
                                } 

                                cmd.args.payload_len = index;
                                len--;

                                while (len >= 0) {
                                        byte[3] = argv[i+j][len--];
                                        
                                        if (len < 0) {
                                                byte[2] = 0;
                                        } else {
                                                byte[2] = argv[i+j][len--];
                                        }

                                        if (!str_to_uint8(byte, &cmd.args.payload[--index])) {
                                                shell_error(shell,
                                                                "Invalid payload: %s. %s\n",
                                                                argv[i+j],
                                                                ARG_PAYLOAD_HELP);
                                                return -EINVAL;
                                        }
                                }
                        }
                        
                        break;

#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		case ARG_HB_SRC:
                        if (!str_to_uint16(argv[i+j], &cmd.args.hb_src)) {
                                shell_error(shell, "Invalid source address: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }
			
			break;

		case ARG_HB_DST:
                        if (!str_to_uint16(argv[i+j], &cmd.args.hb_dst)) {
                                shell_error(shell, "Invalid destination address: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

			break;

		case ARG_HB_PERIOD:
                        if (!str_to_uint8(argv[i+j], &cmd.args.hb_period)) {
                                shell_error(shell, "Invalid heartbeat period: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }
			break;

		case ARG_HB_COUNT:
                        if (!str_to_uint8(argv[i+j], &cmd.args.hb_count)) {
                                shell_error(shell, "Invalid heartbeat count: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }
			
			break;

		case ARG_HB_MIN:
                        if (!str_to_uint8(argv[i+j], &cmd.args.hb_min)) {
                                shell_error(shell, "Invalid minimum hops: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }
		
			break;

		case ARG_HB_MAX:
                        if (!str_to_uint8(argv[i+j], &cmd.args.hb_max)) {
                                shell_error(shell, "Invalid maximum hops: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }
			
			break;

		case ARG_HB_RELAY_STATE:
                        if (!str_to_state(argv[i+j], &cmd.args.hb_relay_state)) {
                                shell_error(shell, "Invalid heartbeat relay state: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

			break;

		case ARG_HB_PROXY_STATE:
                        if (!str_to_state(argv[i+j], &cmd.args.hb_proxy_state)) {
                                shell_error(shell, "Invalid heartbeat proxy state: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

			break;

		case ARG_HB_FRIEND_STATE:
                        if (!str_to_state(argv[i+j], &cmd.args.hb_friend_state)) {
                                shell_error(shell, "Invalid heartbeat friend state: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

			break;

		case ARG_HB_LPN_STATE:
                        if (!str_to_state(argv[i+j], &cmd.args.hb_lpn_state)) {
                                shell_error(shell, "Invalid heartbeat LPN state: %s. %s\n", argv[i+j],
                                                ARG_OPCODE_HELP);
                                return -EINVAL;
                        }

			break;
#endif // CONFIG_SHELL_MESH_HEARTBEAT

                default:
                        break; 
                }
        }

        return 0;    
}

static bool cmd_err_handler(const struct shell *shell, int err, uint8_t status)
{
        if (err) {
                shell_error(shell, "Operation failed. Error code: %d\n", err);
                return true;
        }

        /* Switch for commands that don't require status checking */
        switch (cmd.cmd) {
                case CMD_BEACON_BLOCK:
                case CMD_BEACON_UNBLOCK:
                case CMD_NODE_RESET:
                case CMD_NODE_BEACON_GET:
                case CMD_NODE_TTL_GET:
                case CMD_NODE_FRIEND_GET:
                case CMD_NODE_PROXY_GET:
                case CMD_NODE_RELAY_GET:
                case CMD_NODE_SUBNET_GET:
#ifdef CONFIG_SHELL_MESH_MSG
                case CMD_MSG_SUB:
                case CMD_MSG_UNSUB:
                case CMD_MSG_SEND:
#endif // CONFIG_SHELL_MESH_MSG
#ifdef CONFIG_SHELL_MESH_HEALTH
		case CMD_HLTH_FAULT_GET:
		case CMD_HLTH_FAULT_CLR:
		case CMD_HLTH_FAULT_TEST:
		case CMD_HLTH_PERIOD_GET:
		case CMD_HLTH_PERIOD_SET:
		case CMD_HLTH_ATTN_GET:
		case CMD_HLTH_ATTN_SET:
#endif // CONFIG_SHELL_MESH_HEALTH
                        return false;
                default:
                        break;
        }

        /* Switch for checking feature set status */
        switch (cmd.cmd) {
                case CMD_NODE_TTL_SET:
                        if (status != cmd.args.ttl) {
                                return true;
                        } else {
                                return false;
                        }

                case CMD_NODE_BEACON_SET:
                case CMD_NODE_FRIEND_SET:
                case CMD_NODE_PROXY_SET:
                case CMD_NODE_RELAY_SET:
                        if (status != cmd.args.state) {
                                if (status == FEATURE_DISABLED) {
                                        shell_error(shell, "%s DISABLED\n", STATUS_ERR);
                                } else if (status == FEATURE_ENABLED) {
                                        shell_error(shell, "%s ENABLED\n", STATUS_ERR);
                                } else if (status == FEATURE_NOT_SUPPORTED) {
                                        shell_error(shell, "%s NOT SUPPORTED\n", STATUS_ERR);
                                } else {
                                        shell_error(shell, "%s %d\n", STATUS_ERR, status);
                                }

                                return true;
                        } else {
                                return false;
                        }

                default:
                        break;
        }

        /* Switch for checking mesh status codes */
        switch (status) {
                case STATUS_SUCCESS:
                        return false;

                case STATUS_INVALID_ADDRESS:
                        shell_error(shell, "%s INVALID ADDRESS\n", STATUS_ERR);
                        break;

                case STATUS_INVALID_MODEL:
                        shell_error(shell, "%s INVALID MODEL\n", STATUS_ERR);
                        break;

                case STATUS_INVALID_APPKEY_INDEX:
                        shell_error(shell, "%s INVALID APPLICATION KEY INDEX\n",
                                        STATUS_ERR);
                        break;

                case STATUS_INVALID_NETKEY_INDEX:
                        shell_error(shell, "%s INVALID NETWORK KEY INDEX\n", STATUS_ERR);
                        break;

                case STATUS_INSUFFICIENT_RESOURCES:
                        shell_error(shell, "%s INSUFFICIENT RESOURCES\n", STATUS_ERR);
                        break;

                case STATUS_KEY_INDEX_ALREADY_STORED:
                        shell_error(shell, "%s KEY INDEX ALREADY STORED\n", STATUS_ERR);
                        break;

                case STATUS_INVALID_PUBLISH_PARAMETERS:
                        shell_error(shell, "%s INVALID PUBLISH PARAMETERS\n", STATUS_ERR);
                        break;

                case STATUS_NOT_A_SUBSCRIBE_MODEL:
                        shell_error(shell, "%s NOT A SUBSCRIBE MODEL\n", STATUS_ERR);
                        break;

                case STATUS_STORAGE_FAILURE:
                        shell_error(shell, "%s STORAGE FAILURE\n", STATUS_ERR);
                        break;

                case STATUS_FEATURE_NOT_SUPPORTED:
                        shell_error(shell, "%s FEATURE NOT SUPPORTED\n", STATUS_ERR);
                        break;

                case STATUS_CANNOT_UPDATE:
                        shell_error(shell, "%s CANNOT UPDATE\n", STATUS_ERR);
                        break;

                case STATUS_CANNOT_REMOVE:
                        shell_error(shell, "%s CANNOT REMOVE\n", STATUS_ERR);
                        break;

                case STATUS_CANNOT_BIND:
                        shell_error(shell, "%s CANNOT BIND\n", STATUS_ERR);
                        break;

                case STATUS_TEMPORARILY_UNABLE_TO_CHANGE_STATUS:
                        shell_error(shell, "%s TEMPORARILY UNABLE TO CHANGE STATUS\n",
                                        STATUS_ERR);
                        break;

                case STATUS_CANNOT_SET:
                        shell_error(shell, "%s CANNOT SET\n", STATUS_ERR);
                        break;

                case STATUS_UNSPECIFIED_ERROR:
                        shell_error(shell, "%s UNSPECIFIED ERROR\n", STATUS_ERR);
                        break;

                case STATUS_INVALID_BINDING:
                        shell_error(shell, "%s INVALID BINDING\n", STATUS_ERR);
                        break;

                default:
                        shell_error(shell, "%s UNKOWN STATUS: %d\n", STATUS_ERR, status);
        }

        return true;
}

static bool cmd_print_args(const struct shell *shell, size_t argc, char **argv)
{
        uint8_t i;
        uint8_t arg_start;
        char *arg_str;

        /* Set the start of arguments within argv */
        switch (cmd.cmd) {
                case CMD_BEACON_LIST:
                case CMD_BEACON_BLOCK:
                case CMD_BEACON_UNBLOCK:
                case CMD_NODE_LIST:
                case CMD_NODE_RESET:
                case CMD_NODE_DISC:
                case CMD_NODE_COMP:
#ifdef CONFIG_SHELL_MESH_MSG
                case CMD_MSG_SUB:
                case CMD_MSG_UNSUB:
                case CMD_MSG_SEND:
#endif // CONFIG_SHELL_MESH_MSG
                        arg_start = 1;
                        break;
                case CMD_NODE_BEACON_GET:
                case CMD_NODE_BEACON_SET:
                case CMD_NODE_TTL_GET:
                case CMD_NODE_TTL_SET:
                case CMD_NODE_FRIEND_GET:
                case CMD_NODE_FRIEND_SET:
                case CMD_NODE_PROXY_GET:
                case CMD_NODE_PROXY_SET:
                case CMD_NODE_RELAY_GET:
                case CMD_NODE_RELAY_SET:
                case CMD_NODE_SUBNET_ADD:
                case CMD_NODE_SUBNET_GET:
                case CMD_NODE_SUBNET_DEL:
                case CMD_NODE_CONFIG_SET:
                case CMD_NODE_CONFIG_UNSET:
#ifdef CONFIG_SHELL_MESH_SIG
                case CMD_MODEL_APP_KEY_BIND:
                case CMD_MODEL_APP_KEY_UNBIND:
                case CMD_MODEL_APP_KEY_GET:
                case CMD_MODEL_PUB_GET:
                case CMD_MODEL_PUB_SET:
                case CMD_MODEL_SUB_ADD:
                case CMD_MODEL_SUB_DEL:
                case CMD_MODEL_SUB_OVRW:
                case CMD_MODEL_SUB_GET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
                case CMD_MODEL_APP_KEY_BIND_VND:
                case CMD_MODEL_APP_KEY_UNBIND_VND:
                case CMD_MODEL_APP_KEY_GET_VND:
                case CMD_MODEL_PUB_GET_VND:
                case CMD_MODEL_PUB_SET_VND:
                case CMD_MODEL_SUB_ADD_VND:
                case CMD_MODEL_SUB_DEL_VND:
                case CMD_MODEL_SUB_OVRW_VND:
                case CMD_MODEL_SUB_GET_VND:
#endif // CONFIG_SHELL_MESH_VND
                        arg_start = 2;
                        break;
                default:
                        /* Should be imposible */
                        arg_start = UINT8_MAX;
                        break;
        }

        i = 0;

        while ((cmd.arg_list[i] != ARG_TYPES_END) && (arg_start + i < argc)) {
                arg_str = argv[arg_start + i];

                switch (cmd.arg_list[i])
                {
                case ARG_UUID:
                        shell_print(shell, "UUID     : %s", arg_str);
                        break;

                case ARG_ADDR:
                        shell_print(shell, "addr     : %s", arg_str);
                        break;

                case ARG_STATE:
                        shell_print(shell, "state    : %s", arg_str);
                        break;

                case ARG_TTL:
                        shell_print(shell, "ttl      : %s", arg_str);
                        break;

                case ARG_COUNT:
                        shell_print(shell, "count    : %s", arg_str);
                        break;

                case ARG_INTERVAL:
                        shell_print(shell, "interval : %s", arg_str);
                        break;

                case ARG_NET_IDX:
                        shell_print(shell, "netIdx   : %s", arg_str);
                        break;

                case ARG_APP_IDX:
                        shell_print(shell, "appIdx   : %s", arg_str);
                        break;

                case ARG_ELEM_ADDR:
                        shell_print(shell, "elemAddr : %s", arg_str);
                        break;

                case ARG_MODEL_ID:
                        shell_print(shell, "modelId  : %s", arg_str);
                        break;

                case ARG_CID:
                        shell_print(shell, "cid      : %s", arg_str);
                        break;

		case ARG_TEST_ID:
			shell_print(shell, "testId   : %s", arg_str);
			break;

		case ARG_DIV:
			shell_print(shell, "divisor  : %s", arg_str);
			break;

		case ARG_ATTN:
			shell_print(shell, "attention: %s", arg_str);
			break;

		case ARG_TIMEOUT:
			shell_print(shell, "timeout  : %s", arg_str);
			break;

                case ARG_PUB_ADDR:
                        shell_print(shell, "pubAddr  : %s", arg_str);
                        break;

                case ARG_PUB_APP_IDX:
                        shell_print(shell, "pubAppIdx: %s", arg_str);
                        break;

                case ARG_CRED_FLAG:
                        shell_print(shell, "credFlag : %s", arg_str);
                        break;

                case ARG_PERIOD_KEY:
                        shell_print(shell, "periodKey: %s", arg_str);
                        break;

                case ARG_PERIOD:
                        shell_print(shell, "period   : %s", arg_str);
                        break;

                case ARG_SUB_ADDR:
                        shell_print(shell, "subAddr  : %s", arg_str);
                        break;

                case ARG_OPCODE:
                        shell_print(shell, "opcode   : %s", arg_str);
                        break;

                case ARG_PAYLOAD:
                        shell_print(shell, "payload  : %s", arg_str);
                        break;

#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		case ARG_HB_SRC:
                        shell_print(shell, "src      : %s", arg_str);
			break;

		case ARG_HB_DST:
                        shell_print(shell, "dest     : %s", arg_str);
			break;

		case ARG_HB_PERIOD:
			shell_print(shell, "period   : %s", arg_str);
			break;

		case ARG_HB_COUNT:
			shell_print(shell, "count    : %s", arg_str);
			break;

		case ARG_HB_MIN:
			shell_print(shell, "min      : %s", arg_str);
			break;

		case ARG_HB_MAX:
			shell_print(shell, "max      : %s", arg_str);
			break;

		case ARG_HB_RELAY_STATE:
			shell_print(shell, "relay   : %s", arg_str);
			break;

		case ARG_HB_PROXY_STATE:
			shell_print(shell, "proxy   : %s", arg_str);
			break;

		case ARG_HB_FRIEND_STATE:
			shell_print(shell, "friend  : %s", arg_str);
			break;

		case ARG_HB_LPN_STATE:
			shell_print(shell, "lpn     : %s", arg_str);
			break;
#endif // CONFIG_SHELL_MESH_HEARTBEAT

                default:
                        shell_error(shell, "Unrecognized Argument");
                        return true;
                }

                i++;
        }

        shell_print(shell, "");
        return false;
}

#define ENABLED_STR "ENABLED"
#define DISABLED_STR "DISABLED"
#define SUPPORTED_STR "SUPPORTED"
#define NOT_SUPPORTED_STR "NOT SUPPORTED"

static char *get_state_str(bool state)
{
        if (state) {
                return ENABLED_STR;
        } else {
                return DISABLED_STR;
        }
}

static char *get_support_str(bool support)
{
        if (support) {
                return SUPPORTED_STR;
        } else {
                return NOT_SUPPORTED_STR;
        }
}

static int cmd_handler(const struct shell *shell, size_t argc, char **argv)
{
        int err;
        size_t i;
        size_t j;
        size_t k;
        const char *uuid;
        const char *oob;
        uint8_t uuid_bytes[UUID_LEN];
        const uint32_t *uri_hash;
        uint16_t features;
        uint8_t num_elem;
        uint8_t num_sig_models;
        uint8_t num_vnd_models;
        uint8_t status;
#if defined(CONFIG_SHELL_MESH_SIG) || defined(CONFIG_SHELL_MESH_VND)
        struct bt_mesh_cdb_app_key *appkey;
#endif // defined(CONFIG_SHELL_MESH_SIG) || dfined(CONFIG_SHELL_MESH_VND)
        struct bt_mesh_cdb_subnet *subnet;
        struct bt_mesh_cdb_node *cdb_node;
        struct btmesh_node node;
        char uuid_str[UUID_STR_LEN];
        struct btmesh_elem *elem;
        struct btmesh_sig_model *sig_model;
        struct btmesh_vnd_model *vnd_model;
        union btmesh_op_args op_args;

	memset(&op_args, 0, sizeof(op_args));

        err = cmd_parse_cmd(shell, argv);

        if (err) {
                return err;
        }

        err = cmd_parse_args(shell, argc, argv);

        if (err) {
                return err;
        }

        err = cmd_print_args(shell, argc, argv);

        if (err) {
                return err;
        }

        switch (cmd.cmd) {
	case CMD_BEACON_LIST:
		i = 0;
		uuid = btmesh_get_beacon_uuid(i);
		oob = btmesh_get_beacon_oob(i);
		uri_hash = btmesh_get_beacon_uri_hash(i);

		if (uuid == NULL) {
			shell_print(shell, "  No active beacons.\n");
			break;
		}

		while (uuid != NULL) {
			shell_print(shell,
					"  UUID: %s\n"
					"  - OOB Info: %s",
					uuid, oob ? oob : "None");

			if (uri_hash == NULL) {
				shell_print(shell, "  - URI Hash: None\n");
			} else {
				shell_print(shell, "  - URI Hash: 0x%08x\n", *uri_hash);
			}

			i++;
			uuid = btmesh_get_beacon_uuid(i);
			oob = btmesh_get_beacon_oob(i);
			uri_hash = btmesh_get_beacon_uri_hash(i);
		}

		break;

	case CMD_BEACON_BLOCK:
		err = btmesh_block_beacon(cmd.args.uuid);

		if (cmd_err_handler(shell, err, status)) { 
			if (err == -ENOMEM) {
				shell_info(shell,
						"  Blocked beacon list is full. Cannot add more "
						"UUIDs to blocked list.\n");
				break;
			}

			if (err == -EAGAIN) {
				shell_info(shell,
						"  UUID is already on blocked beacon list.\n");
				break;
			}
		}

		shell_info(shell, "  Added %s to blocked beacon list\n", argv[1]);
		break;

	case CMD_BEACON_UNBLOCK:
		err = btmesh_unblock_beacon(cmd.args.uuid);

		if (cmd_err_handler(shell, err, status)) {
			shell_info(shell, "  Beacon NOT unblocked.\n");
			break;
		}

		shell_info(shell,
				"  Removed %s from blocked beacon list\n", argv[1]);
		break;

	case CMD_NODE_LIST:
		shell_info(shell, "  Network Nodes:");
		bt_mesh_cdb_node_foreach(node_print, (void*)shell);
		break;

	case CMD_NODE_RESET:
		op_args.node_reset.net_idx = PRIMARY_SUBNET;
		op_args.node_reset.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_NODE_RESET, &op_args);

		/* Zephyr node reset API returns errors and bad statuses most of the
		 * time even if the node has been successfully reset. So, we check
		 * to see if the node is now beaconing as unprovisioned to determine
		 * whether or not the reset was a success. */
		cdb_node = bt_mesh_cdb_node_get(op_args.node_reset.addr);

		if (cdb_node == NULL) {
			shell_error(shell, "Failed to remove node from CDB\n");
			shell_info(shell, "Node NOT reset.\n");
			return -ENOEXEC;
		}

		k_sleep(K_SECONDS(5));

		i = 0;
		uuid = btmesh_get_beacon_uuid(i);

		while (uuid != NULL) {
			util_str2uuid(uuid, uuid_bytes);
			if (!util_uuid_cmp(uuid_bytes, cdb_node->uuid)) {
				bt_mesh_cdb_node_del(cdb_node, true);
				shell_info(shell, "Successfully reset node %s\n", argv[1]);
				break;
			}

			i++; 
			uuid = btmesh_get_beacon_uuid(i);
		}

		/* If the reset was a success, we just deleted this node from the
		 * CDB and so its address should be set to unassigned. If so, we're
		 * done here. */
		if (cdb_node->addr == BT_MESH_ADDR_UNASSIGNED) {
			break;
		}

		/* The node being reset cannot send an ack message after resetting
		 * itself so the call to bt_mesh_cfg_node_reset() always returns
		 * err -11 (-EAGAIN). Ignore this and only check status for the
		 * success/failure of the reset */
		if (err && err != -EAGAIN)
		{
			shell_error(shell, "Error: %d\n", err);
			shell_info(shell, "Node NOT reset\n");
			return -ENOEXEC;
		}

		if (op_args.node_reset.status) {
			shell_error(shell, "Status: %d\n", op_args.node_reset.status);
			shell_info(shell, "Node NOT reset\n");
			return -ENOEXEC;
		}

		break;

	case CMD_NODE_DISC:
		node.addr = cmd.args.addr;
		err = btmesh_discover_node(&node, &status);

		if (cmd_err_handler(shell, err, status)) {
			return -ENOEXEC;
		}

		util_uuid2str(node.uuid, uuid_str);

		shell_print(shell,
				"  Node Network Details\n"
				"    - UUID     : %s\n"
				"    - Address  : 0x%04x\n"
				"    - Net Index: 0x%04x\n",
				uuid_str, node.addr, node.net_idx);

		/* Print composition data */
		shell_print(shell,
				"  Node Identifiers\n"
				"    - CID : 0x%04x\n"
				"    - PID : 0x%04x\n"
				"    - VID : 0x%04x\n"
				"    - CRPL: 0x%04x\n",
				node.cid, node.pid, node.vid, node.crpl);

		/* Print node's features */ 
		shell_print(shell,
				"  Node Features\n"
				"    - Network Beacon:\n"
				"        %s\n"
				"    - Time-to-live:\n"
				"        %d\n"
				"    - Relay Feature:\n"
				"        %s\n"
				"        %s\n"
				"        Retransmit Count: %d\n"
				"        Retransmit Interval: %d\n"
				"    - GATT Proxy Feature:\n"
				"        %s\n"
				"        %s\n"
				"    - Friend Feature:\n"
				"        %s\n"
				"        %s\n"
				"    - Low-Power-Node Feature:\n"
				"        %s\n",
				get_state_str(node.net_beacon_state), node.ttl,
				get_support_str(node.relay.support),
				get_state_str(node.relay.state),
				node.relay.count, node.relay.interval,
				get_support_str(node.proxy.support),
				get_state_str(node.proxy.state),
				get_support_str(node.friend.support),
				get_state_str(node.friend.state),
				get_state_str(node.lpn));

		/* Print node's heartbeat subscribe parameters */
		shell_print(shell,
				"  Node Heartbeat Subscribe Parameters\n"
				"    - Source     : 0x%04x\n"
				"    - Destination: 0x%04x\n"
				"    - Period     : 0x%02x\n"
				"    - Count      : 0x%02x\n"
				"    - Min Hops   : 0x%02x\n"
				"    - Max Hops   : 0x%02x\n",
				node.hb_sub.src, node.hb_sub.dst, node.hb_sub.period,
				node.hb_sub.count, node.hb_sub.min, node.hb_sub.max);

		/* Print node's heartbeat publish parameters */
		shell_print(shell,
				"  Node Heartbeat Publish Parameters\n"
				"    - Destination     : 0x%04x\n"
				"    - Count           : 0x%02x\n"
				"    - Period          : 0x%02x\n"
				"    - Time-to-Live    : 0x%02x\n"
				"    - Relay Heartbeat : %s\n"
				"    - Proxy Heartbeat : %s\n"
				"    - Friend Heartbeat: %s\n"
				"    - LPN Heartbeat   : %s\n"
				"    - Net Index       : 0x%04x\n",
				node.hb_pub.dst, node.hb_pub.count, node.hb_pub.period,
				node.hb_pub.ttl,
				get_state_str(node.hb_pub.feat & BT_MESH_FEAT_RELAY),
				get_state_str(node.hb_pub.feat & BT_MESH_FEAT_PROXY),
				get_state_str(node.hb_pub.feat & BT_MESH_FEAT_FRIEND),
				get_state_str(node.hb_pub.feat & BT_MESH_FEAT_LOW_POWER),
				node.hb_pub.net_idx);

		/* Print node's subnets */
		shell_print(shell, "  Node Subnets:");
		for (i = 0; i < node.subnet_count; i++) {
			shell_print(shell, "    - 0x%04x", node.subnet_idxs[i]);
		}
		shell_print(shell, "");

		/* Print node's elements */
		for (i = 0; i < node.elem_count; i++) {
			elem = &node.elems[i];
			shell_print(shell,
					"  Element %d %s\n"
					"    - Address: 0x%04x\n"
					"    - LOC    : 0x%04x\n",
					i + 1, (i==0) ? "(Primary)" : "", elem->addr,
					elem->loc);

			/* Print element's SIG models */
			shell_print(shell, "      SIG Models");
			for (j = 0; j < elem->sig_model_count; j++) {
				sig_model = &elem->sig_models[j];
				shell_print(shell,
						"        Model ID: 0x%04x %s\n"
						"          Application Keys:",
						sig_model->model_id,
						btmesh_get_sig_model_str(sig_model->model_id));

				/* Print SIG model's application key indexes */
				for (k = 0; k < sig_model->appkey_count; k++) {
					shell_print(shell, "          - 0x%04x",
							sig_model->appkey_idxs[k]);
				}

				if (k == 0) {
					shell_print(shell, "            None");
				}

				/* Print SIG model's subscribe addresses */
				shell_print(shell, "          Subscribed Addresses:");

				for (k = 0; k < sig_model->sub_addr_count; k++) {
					shell_print(shell, "          - 0x%04x",
							sig_model->sub_addrs[k]);
				}

				if (k == 0) {
					shell_print(shell, "            None");
				}

				/* Print SIG model's publish parameters */
				shell_print(shell,
						"          Publish Parameters:\n"
						"          - Address                   : 0x%04x\n"
						"          - Application Index         : 0x%04x\n"
						"          - Friendship Credential Flag: %s\n"
						"          - Time-to-live              : %d\n"
						"          - Period                    : %d (%s)\n"
						"          - Retransmit Count          : %d\n"
						"          - Retransmit Interval       : %d\n",
						sig_model->pub.addr, sig_model->pub.app_idx,
						get_state_str(sig_model->pub.cred_flag),
						sig_model->pub.ttl,
						btmesh_get_pub_period(sig_model->pub.period),
						btmesh_get_pub_period_unit_str(
							sig_model->pub.period),
						BT_MESH_TRANSMIT_COUNT(sig_model->pub.transmit),
						BT_MESH_TRANSMIT_INT(sig_model->pub.transmit));
			}

			if (j == 0) {
				shell_print(shell, "        None\n");
			}

			/* Print element's vendor models */
			shell_print(shell, "      Vendor Models:");

			for (j = 0; j < elem->vnd_model_count; j++) {
				vnd_model = &elem->vnd_models[j];
				shell_print(shell,
						"        Company ID: 0x%04x\n"
						"        Model ID  : 0x%04x\n"
						"          Application Keys:",
						vnd_model->company_id, vnd_model->model_id);

				/* Print vendor model's application key indexes */
				for (k = 0; k < vnd_model->appkey_count; k++) {
					shell_print(shell, "          -  0x%04x",
							vnd_model->appkey_idxs[k]);
				}

				if (k == 0) {
					shell_print(shell, "            None");
				}

				/* Print vendor model's subscribe addresses */
				shell_print(shell, "          Subscribe Addresses:");

				for (k = 0; k < vnd_model->sub_addr_count; k++) {
					shell_print(shell, "          - 0x%04x",
							vnd_model->sub_addrs[k]);
				}

				if (k == 0) {
					shell_print(shell, "            None");
				}

				/* Print vendor model's publish parameters */
				shell_print(shell,
						"          Publish Parameters:\n"
						"          - Address                   : 0x%04x\n"
						"          - Application Index         : 0x%04x\n"
						"          - Friendship Credential Flag: %s\n"
						"          - Time-to-live              : %d\n"
						"          - Period                    : %d (%s)\n"
						"          - Retransmit Count          : %d\n"
						"          - Retransmit Interval       : %d\n",
						vnd_model->pub.addr, vnd_model->pub.app_idx,
						get_state_str(vnd_model->pub.cred_flag),
						vnd_model->pub.ttl,
						btmesh_get_pub_period(vnd_model->pub.period),
						btmesh_get_pub_period_unit_str(
							vnd_model->pub.period),
						BT_MESH_TRANSMIT_COUNT(vnd_model->pub.transmit),
						BT_MESH_TRANSMIT_INT(vnd_model->pub.transmit));
			}

			if (j == 0) {
				shell_print(shell, "        None\n");
			}
		}

		btmesh_free_node(&node);
		break;

	case CMD_NODE_COMP:
		op_args.comp_get.net_idx = PRIMARY_SUBNET;
		op_args.comp_get.addr = cmd.args.addr;
		op_args.comp_get.page = 0x00;
		op_args.comp_get.comp = NET_BUF_SIMPLE(64);
		net_buf_simple_init(op_args.comp_get.comp, 0);
		err = btmesh_perform_op(BTMESH_OP_COMP_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.comp_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node's composition data:");
		shell_print(shell,
				"  CID : 0x%04x\n"
				"  PID : 0x%04x\n"
				"  VID : 0x%04x\n"
				"  CRPL: 0x%04x\n"
				"  Features:",
				net_buf_simple_pull_le16(op_args.comp_get.comp),
				net_buf_simple_pull_le16(op_args.comp_get.comp),
				net_buf_simple_pull_le16(op_args.comp_get.comp),
				net_buf_simple_pull_le16(op_args.comp_get.comp));
		features = net_buf_simple_pull_le16(op_args.comp_get.comp);

		if (features & 0x0001) {
			shell_print(shell, "    - Relay");
		}

		if (features & 0x0002) {
			shell_print(shell, "    - Proxy");
		}

		if (features & 0x0004) {
			shell_print(shell, "    - Friend");
		}

		if (features & 0x0008) {
			shell_print(shell, "    - Low Power");
		}

		num_elem = bt_mesh_cdb_node_get(op_args.comp_get.addr)->num_elem;

		for (i = 0; i < num_elem; i++) {
			shell_print(shell, "  LOC: 0x%04x",
					net_buf_simple_pull_le16(op_args.comp_get.comp));
			num_sig_models = net_buf_simple_pull_u8(op_args.comp_get.comp);
			num_vnd_models = net_buf_simple_pull_u8(op_args.comp_get.comp);
			shell_print(shell, "  Element %d (0x%04x):", i + 1, 
					op_args.comp_get.addr + i);
			shell_print(shell, "    SIG Model IDs:");

			for (j = 0; j < num_sig_models; j++) {
				shell_print(shell, "      - 0x%04x",
						net_buf_simple_pull_le16(op_args.comp_get.comp));
			}

			if (j == 0) {
				shell_print(shell, "      None");    
			}

			shell_print(shell, "    Vendor Model IDs:");

			for (j = 0; j < num_vnd_models; j++) {
				shell_print(shell, "      - 0x%08x",
						net_buf_simple_pull_le32(op_args.comp_get.comp));
			}

			if (j == 0) {
				shell_print(shell, "      None");
			}
		}

		shell_print(shell, "");
		break;

	case CMD_NODE_BEACON_GET:
		op_args.beacon_get.net_idx = PRIMARY_SUBNET;
		op_args.beacon_get.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_BEACON_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.beacon_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node's network beacon state:");
		print_feature_status(shell, op_args.beacon_get.status);
		shell_print(shell,"");
		break;

	case CMD_NODE_BEACON_SET:
		op_args.beacon_set.net_idx = PRIMARY_SUBNET;
		op_args.beacon_set.addr = cmd.args.addr;
		op_args.beacon_set.val = cmd.args.state;
		err = btmesh_perform_op(BTMESH_OP_BEACON_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.beacon_set.status)) {
			shell_info(shell, "  Node's beacon feature state NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell, 
				"  Successfully set node's network beacon state:");
		print_feature_status(shell, op_args.beacon_set.status);
		shell_print(shell, "");
		break;

	case CMD_NODE_TTL_GET:
		op_args.ttl_get.net_idx = PRIMARY_SUBNET;
		op_args.ttl_get.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_TTL_GET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell,
				"  Node's time-to-live value: %d\n", op_args.ttl_get.ttl);
		break;

	case CMD_NODE_TTL_SET:
		op_args.ttl_set.net_idx = PRIMARY_SUBNET;
		op_args.ttl_set.addr = cmd.args.addr;
		op_args.ttl_set.val = cmd.args.ttl;
		err = btmesh_perform_op(BTMESH_OP_TTL_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.ttl_set.ttl)) {
			shell_info(shell,
					"  Node's time-to-live feature state NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully set nodes time-to-live state: %d", 
				op_args.ttl_set.ttl);
		break;

	case CMD_NODE_FRIEND_GET:
		op_args.friend_get.net_idx = PRIMARY_SUBNET;
		op_args.friend_get.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_FRIEND_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.friend_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node's friend feature state:");
		print_feature_status(shell, op_args.friend_get.status);
		shell_print(shell, "");
		break;

	case CMD_NODE_FRIEND_SET:
		op_args.friend_set.net_idx = PRIMARY_SUBNET;
		op_args.friend_set.addr = cmd.args.addr;
		op_args.friend_set.val = cmd.args.state;
		err = btmesh_perform_op(BTMESH_OP_FRIEND_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.friend_set.status)) {
			shell_info(shell, "  Node's friend feature state NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell,
				"  Successfully set node's friend feature state:");
		print_feature_status(shell, op_args.friend_set.status); 
		shell_print(shell, "");
		break;

	case CMD_NODE_PROXY_GET:
		op_args.proxy_get.net_idx = PRIMARY_SUBNET;
		op_args.proxy_get.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_PROXY_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.proxy_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node's GATT proxy feature state:");
		print_feature_status(shell, op_args.proxy_get.status); 
		shell_print(shell, "");
		break;

	case CMD_NODE_PROXY_SET:
		op_args.proxy_set.net_idx = PRIMARY_SUBNET;
		op_args.proxy_set.addr = cmd.args.addr;
		op_args.proxy_set.val = cmd.args.state;
		err = btmesh_perform_op(BTMESH_OP_PROXY_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.proxy_set.status)) {
			shell_info(shell, "  Node's proxy feature state NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell,
				"  Successfully set node's GATT proxy feature state.");
		print_feature_status(shell, op_args.proxy_set.status);
		shell_print(shell, "");
		break;

	case CMD_NODE_RELAY_GET:
		op_args.relay_get.net_idx = PRIMARY_SUBNET;
		op_args.relay_get.addr = cmd.args.addr;
		err = btmesh_perform_op(BTMESH_OP_RELAY_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.relay_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node relay feature state:");
		print_feature_status(shell, op_args.relay_get.status);
		shell_print(shell,
				"  - Transmit Count   : %d\n"
				"  - Transmit Interval: %d\n",
				BT_MESH_TRANSMIT_COUNT(op_args.relay_get.transmit),
				BT_MESH_TRANSMIT_INT(op_args.relay_get.transmit));
		break;

	case CMD_NODE_RELAY_SET:
		op_args.relay_set.net_idx = PRIMARY_SUBNET;
		op_args.relay_set.addr = cmd.args.addr;
		op_args.relay_set.new_relay = cmd.args.state;
		op_args.relay_set.new_transmit = BT_MESH_TRANSMIT(cmd.args.count,
				cmd.args.interval);
		err = btmesh_perform_op(BTMESH_OP_RELAY_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.relay_set.status)) {
			shell_info(shell, "  Node's relay feature state NOT set!\n");
			return -ENOEXEC;
		}

		if (op_args.relay_set.transmit != op_args.relay_set.new_transmit) {
			shell_error(shell, "Relay Transmit Error.\n");
			shell_info(shell, "  Node's relay feature state NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell,
				"  Successfully set node's relay feature state.\n");
		break;

#ifdef CONFIG_SHELL_MESH_HEARTBEAT
	case CMD_NODE_HB_SUB_SET:
		op_args.hb_sub_set.net_idx = PRIMARY_SUBNET;
		op_args.hb_sub_set.addr = cmd.args.addr;
		op_args.hb_sub_set.sub.src = cmd.args.hb_src;
		op_args.hb_sub_set.sub.dst = cmd.args.hb_dst;
		op_args.hb_sub_set.sub.period = cmd.args.hb_period;
		op_args.hb_sub_set.sub.min = cmd.args.hb_min;
		op_args.hb_sub_set.sub.max = cmd.args.hb_max;
		err = btmesh_perform_op(BTMESH_OP_HB_SUB_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.hb_sub_set.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node heartbeat subscribe parameters successfully set\n");
		
		break;

	case CMD_NODE_HB_PUB_SET:
		op_args.hb_pub_set.net_idx = PRIMARY_SUBNET;
		op_args.hb_pub_set.addr = cmd.args.addr;
		op_args.hb_pub_set.pub.dst = cmd.args.hb_dst;
		op_args.hb_pub_set.pub.count = cmd.args.hb_count;
		op_args.hb_pub_set.pub.period = cmd.args.hb_period;
		op_args.hb_pub_set.pub.ttl = cmd.args.ttl;
		op_args.hb_pub_set.pub.feat =
			(cmd.args.hb_relay_state ? BT_MESH_FEAT_RELAY : 0) |
			(cmd.args.hb_proxy_state ? BT_MESH_FEAT_PROXY : 0) |
			(cmd.args.hb_friend_state ? BT_MESH_FEAT_FRIEND : 0) |
			(cmd.args.hb_lpn_state ? BT_MESH_FEAT_LOW_POWER : 0);
		op_args.hb_pub_set.pub.net_idx = cmd.args.net_idx;
		err = btmesh_perform_op(BTMESH_OP_HB_PUB_SET, &op_args);

		if (cmd_err_handler(shell, err, op_args.hb_pub_set.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node heartbeat publish parameters successfully set\n");

		break;
#endif // CONFIG_SHELL_MESH_HEARTBEAT

	case CMD_NODE_SUBNET_ADD:
		subnet = bt_mesh_cdb_subnet_get(cmd.args.net_idx);

		if (subnet == NULL) {
			shell_error(shell, "Subnet index does not exist. You must"
					" add subnet keys to the gateway before adding them"
					" to a node.\n");
			return -EINVAL;
		}

		op_args.net_key_add.net_idx = PRIMARY_SUBNET;
		op_args.net_key_add.addr = cmd.args.addr;
		op_args.net_key_add.key_net_idx = subnet->net_idx;
		memcpy(op_args.net_key_add.net_key, subnet->keys[0].net_key, KEY_LEN);
		err = btmesh_perform_op(BTMESH_OP_NET_KEY_ADD, &op_args);

		if (cmd_err_handler(shell, err, op_args.net_key_add.status)) {
			shell_info(shell, "  Subnet NOT added to node!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully added subnet to node.\n");
		break;

	case CMD_NODE_SUBNET_GET:
		op_args.net_key_get.net_idx = PRIMARY_SUBNET;
		op_args.net_key_get.addr = cmd.args.addr;
		op_args.net_key_get.key_cnt = sizeof(op_args.net_key_get.keys);
		err = btmesh_perform_op(BTMESH_OP_NET_KEY_GET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Node Subnets:");

		for (i = 0; i < op_args.net_key_get.key_cnt; i++) {
			shell_print(shell, "  - 0x%04x", op_args.net_key_get.keys[i]);
		}

		if (i == 0) {
			shell_print(shell, "    None");
		}

		shell_print(shell, "");
		break;

	case CMD_NODE_SUBNET_DEL:
		op_args.net_key_del.net_idx = PRIMARY_SUBNET;
		op_args.net_key_del.addr = cmd.args.addr;
		op_args.net_key_del.key_net_idx = cmd.args.net_idx;
		err = btmesh_perform_op(BTMESH_OP_NET_KEY_DEL, &op_args);

		if (cmd_err_handler(shell, err, op_args.net_key_del.status)) {
			shell_info(shell, "  Subnet NOT deleted from node!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully deleted subnet from node.\n");
		break;

	case CMD_NODE_CONFIG_SET:
		cdb_node = bt_mesh_cdb_node_get(cmd.args.addr);

		if (cdb_node == NULL) {
			shell_error(shell, "Failed to get node from CDB\n");
			shell_info(shell, "Node configure flag NOT set\n");
			return -ENOEXEC;
		}

		atomic_set_bit(cdb_node->flags, BT_MESH_CDB_NODE_CONFIGURED);

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_cdb_node_store(cdb_node);
		}

		break;

	case CMD_NODE_CONFIG_UNSET:
		cdb_node = bt_mesh_cdb_node_get(cmd.args.addr);

		if (cdb_node == NULL) {
			shell_error(shell, "Failed to get node from CDB\n");
			shell_info(shell, "Node configure flag NOT unset\n");
			return -ENOEXEC;
		}

		atomic_clear_bit(cdb_node->flags, BT_MESH_CDB_NODE_CONFIGURED);

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_cdb_node_store(cdb_node);
		}

		break;

#if defined(CONFIG_SHELL_MESH_SIG) || defined(CONFIG_SHELL_MESH_VND)
#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_APP_KEY_BIND:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_APP_KEY_BIND_VND:
#endif // CONFIG_SHELL_MESH_VND
		/* Make sure that the appkey exists on the gateway */
		appkey = bt_mesh_cdb_app_key_get(cmd.args.app_idx);


		if (appkey == NULL) {
			shell_error(shell,
					"Application index does not exist. You must"
					" add application keys to the gateway before binding"
					" them to models.\n");
			return -EINVAL;
		}

		/* Check to see if the appkey exists on the node */
		op_args.app_key_get.net_idx = PRIMARY_SUBNET;
		op_args.app_key_get.addr = cmd.args.addr;
		op_args.app_key_get.key_net_idx = appkey->net_idx;
		op_args.app_key_get.key_cnt = sizeof(op_args.app_key_get.keys);
		err = btmesh_perform_op(BTMESH_OP_APP_KEY_GET, &op_args);

		if (cmd_err_handler(shell, err, op_args.app_key_get.status)) {
			shell_error(shell, "Failed to query node application keys.\n");
			shell_info(shell, "  Application key NOT bound to model!\n");
			return -ENOEXEC;
		}

		for (i = 0; i < op_args.app_key_get.key_cnt; i++) {
			if (cmd.args.app_idx == op_args.app_key_get.keys[i]) {
				break;
			}
		}

		/* Node doesn't have appkey yet so add it */
		if (i >= op_args.app_key_get.key_cnt) {
			op_args.app_key_add.key_app_idx = appkey->app_idx;
			memcpy(op_args.app_key_add.app_key, appkey->keys[0].app_key, KEY_LEN);
			err = btmesh_perform_op(BTMESH_OP_APP_KEY_ADD, &op_args);

			if (cmd_err_handler(shell, err, op_args.app_key_add.status)) {
				shell_error(shell, "Failed to add application key to node.");
				shell_info(shell, "  Application key NOT bound to model!\n");
				return -ENOEXEC;
			}
		}

		op_args.mod_app_bind.elem_addr = cmd.args.elem_addr;
		op_args.mod_app_bind.mod_app_idx = cmd.args.app_idx;
		op_args.mod_app_bind.mod_id = cmd.args.model_id;
	
#ifdef CONFIG_SHELL_MESH_SIG	
		if (cmd.cmd == CMD_MODEL_APP_KEY_BIND) {
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_BIND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_APP_KEY_BIND_VND) {
			op_args.mod_app_bind_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_BIND_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_app_bind.status)) {
			shell_info(shell, "  Application key NOT binded to model!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully binded application key to model.\n");

		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_APP_KEY_UNBIND:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_APP_KEY_UNBIND_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_app_unbind.net_idx = PRIMARY_SUBNET;
		op_args.mod_app_unbind.addr = cmd.args.addr;
		op_args.mod_app_unbind.elem_addr = cmd.args.elem_addr;
		op_args.mod_app_unbind.mod_app_idx = cmd.args.app_idx;
		op_args.mod_app_unbind.mod_id = cmd.args.model_id;
	
#ifdef CONFIG_SHELL_MESH_SIG	
		if (cmd.cmd == CMD_MODEL_APP_KEY_UNBIND) {
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_UNBIND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_APP_KEY_UNBIND_VND) {
			op_args.mod_app_unbind_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_UNBIND_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_app_unbind.status)) {
			shell_info(shell, "  Application key NOT unbinded from model!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully unbinded application key from model.\n");

		err = btmesh_clean_node_key(op_args.mod_app_unbind.addr,
				op_args.mod_app_unbind.mod_app_idx);
		
		if (err) {
			shell_error(shell, "Could not remove unused key from node. Error: %d", err);
			return -ENOEXEC;
		}

		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_APP_KEY_GET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_APP_KEY_GET_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_app_get.net_idx = PRIMARY_SUBNET;
		op_args.mod_app_get.addr = cmd.args.addr;
		op_args.mod_app_get.elem_addr = cmd.args.elem_addr;
		op_args.mod_app_get.mod_id = cmd.args.model_id;
		op_args.mod_app_get.app_cnt = sizeof(op_args.mod_app_get.apps);
	
#ifdef CONFIG_SHELL_MESH_SIG	
		if (cmd.cmd == CMD_MODEL_APP_KEY_GET) {
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_APP_KEY_GET_VND) {
			op_args.mod_app_get_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_APP_GET_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_app_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Model Application Keys:");

		for (i = 0; i < op_args.mod_app_get.app_cnt; i++) {
			shell_print(shell, "  - 0x%04x", op_args.mod_app_get.apps[i]);
		}

		if (i == 0) {
			shell_print(shell, "    None");
		}

		shell_print(shell, "");
		
		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_PUB_GET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_PUB_GET_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_pub_get.net_idx = PRIMARY_SUBNET;
		op_args.mod_pub_get.addr = cmd.args.addr;
		op_args.mod_pub_get.elem_addr = cmd.args.elem_addr;
		op_args.mod_pub_get.mod_id = cmd.args.model_id;

#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_PUB_GET) {
			err = btmesh_perform_op(BTMESH_OP_MOD_PUB_GET, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_PUB_GET_VND) {
			op_args.mod_pub_get_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_PUB_GET_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_pub_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Model Publish Parameters:");
		shell_print(shell,
				"  - Address          : 0x%04x\n"
				"  - Application Index: 0x%04x\n"
				"  - Friend Credential: %s\n"
				"  - Time-to-Live     : 0x%02x\n"
				"  - Period           : 0x%02x\n"
				"  - TX Count         : 0x%02x\n"
				"  - TX Interval      : 0x%04x\n",
				op_args.mod_pub_get.pub.addr,
				op_args.mod_pub_get.pub.app_idx,
				op_args.mod_pub_get.pub.cred_flag ? "Enabled" : "Disabled",
				op_args.mod_pub_get.pub.ttl, op_args.mod_pub_get.pub.period,
				BT_MESH_TRANSMIT_COUNT(op_args.mod_pub_get.pub.transmit),
				BT_MESH_TRANSMIT_INT(op_args.mod_pub_get.pub.transmit));
		break;
	
#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_PUB_SET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_PUB_SET_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_pub_set.net_idx = PRIMARY_SUBNET;
		op_args.mod_pub_set.addr = cmd.args.addr;
		op_args.mod_pub_set.elem_addr = cmd.args.elem_addr;
		op_args.mod_pub_set.mod_id = cmd.args.model_id;

#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_PUB_SET_VND) {
			op_args.mod_pub_set_vnd.cid = cmd.args.cid;
		}
#endif // CONFIG_SHELL_MESH_VND

		op_args.mod_pub_set.pub.addr = cmd.args.pub_addr;
		op_args.mod_pub_set.pub.uuid = NULL;
		op_args.mod_pub_set.pub.app_idx = cmd.args.pub_app_idx;
		op_args.mod_pub_set.pub.cred_flag = cmd.args.cred_flag;
		op_args.mod_pub_set.pub.ttl = cmd.args.ttl;

		if (cmd.args.period == 0) {
			op_args.mod_pub_set.pub.period = 0;
		} else {
			switch (cmd.args.period_key) {
				case PERIOD_KEY_100MS:
					op_args.mod_pub_set.pub.period =
						BT_MESH_PUB_PERIOD_100MS(cmd.args.period);
					break;
				case PERIOD_KEY_1S:
					op_args.mod_pub_set.pub.period =
						BT_MESH_PUB_PERIOD_SEC(cmd.args.period);
					break;
				case PERIOD_KEY_10S:
					op_args.mod_pub_set.pub.period =
						BT_MESH_PUB_PERIOD_10SEC(cmd.args.period);
					break;
				case PERIOD_KEY_10M:
					op_args.mod_pub_set.pub.period =
						BT_MESH_PUB_PERIOD_10MIN(cmd.args.period);
					break;
				default:
					op_args.mod_pub_set.pub.period = 0;
					break;
			}
		}

		op_args.mod_pub_set.pub.transmit = BT_MESH_TRANSMIT(cmd.args.count,
				cmd.args.interval);
#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_PUB_SET) {
			err = btmesh_perform_op(BTMESH_OP_MOD_PUB_SET, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_PUB_SET_VND) {
			err = btmesh_perform_op(BTMESH_OP_MOD_PUB_SET_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_pub_set.status)) {
			shell_info(shell, "  Model publish parameters NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell,
				"  Successfully set model publish parameters.\n");
		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_SUB_ADD:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_SUB_ADD_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_sub_add.net_idx = PRIMARY_SUBNET;
		op_args.mod_sub_add.addr = cmd.args.addr;
		op_args.mod_sub_add.elem_addr = cmd.args.elem_addr;
		op_args.mod_sub_add.sub_addr = cmd.args.sub_addr;
		op_args.mod_sub_add.mod_id = cmd.args.model_id;

#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_SUB_ADD) {
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_ADD, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_SUB_ADD_VND) {
			op_args.mod_sub_add_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_ADD_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_sub_add.status)) {
			shell_info(shell, "  Subscibe address NOT added to model!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully added subscribe address to model.\n");
		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_SUB_DEL:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_SUB_DEL_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_sub_del.net_idx = PRIMARY_SUBNET;
		op_args.mod_sub_del.addr = cmd.args.addr;
		op_args.mod_sub_del.elem_addr = cmd.args.elem_addr;
		op_args.mod_sub_del.sub_addr = cmd.args.sub_addr;
		op_args.mod_sub_del.mod_id = cmd.args.model_id;

#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_SUB_DEL) {
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_DEL, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_SUB_DEL_VND) {
			op_args.mod_sub_del_vnd.cid = cmd.args.model_id;
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_DEL_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_sub_del.status)) {
			shell_info(shell, "  Subscribe address NOT deleted from model!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully deleted subscribe address from model.\n");
		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_SUB_OVRW:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_SUB_OVRW_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_sub_ovrw.net_idx = PRIMARY_SUBNET;
		op_args.mod_sub_ovrw.addr = cmd.args.addr;
		op_args.mod_sub_ovrw.elem_addr = cmd.args.elem_addr;
		op_args.mod_sub_ovrw.sub_addr = cmd.args.sub_addr;
		op_args.mod_sub_ovrw.mod_id = cmd.args.model_id;

#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_SUB_OVRW) {
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_OVRW, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_SUB_OVRW_VND) {
			op_args.mod_sub_ovrw_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_OVRW, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_sub_ovrw.status)) {
			shell_info(shell,
					"  Subscribe address NOT overwritten on model!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Successfully overwrote subscribe address on model.\n");
		break;

#ifdef CONFIG_SHELL_MESH_SIG
	case CMD_MODEL_SUB_GET:
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
	case CMD_MODEL_SUB_GET_VND:
#endif // CONFIG_SHELL_MESH_VND
		op_args.mod_sub_get.net_idx = PRIMARY_SUBNET;
		op_args.mod_sub_get.addr = cmd.args.addr;
		op_args.mod_sub_get.elem_addr = cmd.args.elem_addr;
		op_args.mod_sub_get.mod_id = cmd.args.model_id;
		op_args.mod_sub_get.sub_cnt = sizeof(op_args.mod_sub_get.subs);

#ifdef CONFIG_SHELL_MESH_SIG
		if (cmd.cmd == CMD_MODEL_SUB_GET) {
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_GET, &op_args);
		}
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
		if (cmd.cmd == CMD_MODEL_SUB_GET_VND) {
			op_args.mod_sub_get_vnd.cid = cmd.args.cid;
			err = btmesh_perform_op(BTMESH_OP_MOD_SUB_GET_VND, &op_args);
		}
#endif // CONFIG_SHELL_MESH_VND

		if (cmd_err_handler(shell, err, op_args.mod_sub_get.status)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Model Subscribe Addresses:");

		for (i = 0; i < op_args.mod_sub_get.sub_cnt; i++) {
			shell_print(shell, "  - 0x%04x", op_args.mod_sub_get.subs[i]);
		}

		if (i == 0) {
			shell_print(shell, "    None");
		}

		shell_print(shell, "");
		break;
#endif // defined(CONFIG_SHELL_MESH_SIG) || defined(CONFIG_SHELL_MESH_VND)
#ifdef CONFIG_SHELL_MESH_MSG
	case CMD_MSG_SUB:
		err = btmesh_subscribe(BTMESH_SUB_TYPE_CLI, cmd.args.addr);
		
		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Subscribed to message address.\n");

		break;

	case CMD_MSG_UNSUB:
		btmesh_unsubscribe(BTMESH_SUB_TYPE_CLI, cmd.args.addr);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Unsibscribed from message address.\n");

		break;

	case CMD_MSG_SEND:
	{
		struct bt_mesh_msg_ctx ctx;
		NET_BUF_SIMPLE_DEFINE(buf, MAX_PAYLOAD_LEN);

		ctx.net_idx = cmd.args.net_idx;
		ctx.app_idx = cmd.args.app_idx;
		ctx.addr = cmd.args.addr;
		
		if (cmd.args.opcode & 0xFF0000) {
			net_buf_simple_add_u8(&buf, (uint8_t)(cmd.args.opcode >> 16));
			net_buf_simple_add_le16(&buf, (uint16_t)cmd.args.opcode);
		} else if (cmd.args.opcode & 0xFF00) {
			net_buf_simple_add_be16(&buf, (uint16_t)cmd.args.opcode);
		} else if (cmd.args.opcode & 0xFF) {
			net_buf_simple_add_u8(&buf, (uint8_t)cmd.args.opcode);
		} else {
			shell_error(shell, "Invalid opcode: 0x%08x", cmd.args.opcode);
			return -EINVAL;
		}

		for (i = 0; i < cmd.args.payload_len; i++) {
			net_buf_simple_add_u8(&buf, cmd.args.payload[i]);
		}

		ctx.send_rel = false;
		ctx.send_ttl = BT_MESH_TTL_DEFAULT;
#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
		err = bt_mesh_msg_send(&ctx, &buf, bt_mesh_primary_addr(), NULL, NULL);
#endif // defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}
	}

	break;
#endif // CONFIG_SHELL_MESH_MSG
#ifdef CONFIG_SHELL_MESH_HEALTH
	case CMD_HLTH_FAULT_GET:
		op_args.hlth_fault_get.addr = cmd.args.addr;
		op_args.hlth_fault_get.app_idx = cmd.args.app_idx;
		op_args.hlth_fault_get.cid = cmd.args.cid;
		op_args.hlth_fault_get.fault_count = sizeof(op_args.hlth_fault_get.faults);

		err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_GET, &op_args);
		
		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Test ID: 0x%02x\n  Faults:");
		for (i = 0; i < op_args.hlth_fault_get.fault_count; i++) {
			shell_info(shell, "  - 0x%02x", op_args.hlth_fault_get.faults[i]);
		}
		shell_info(shell, "");
		break;

	case CMD_HLTH_FAULT_CLR:
		op_args.hlth_fault_clear.addr = cmd.args.addr;
		op_args.hlth_fault_clear.app_idx = cmd.args.app_idx;
		op_args.hlth_fault_clear.cid = cmd.args.cid;
		err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_CLR, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		if (op_args.hlth_fault_clear.fault_count != 0) {
			shell_info(shell, "  Faults NOT cleared!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Faults cleared\n");
		break;

	case CMD_HLTH_FAULT_TEST:
		op_args.hlth_fault_test.addr = cmd.args.addr;
		op_args.hlth_fault_test.app_idx = cmd.args.app_idx;
		op_args.hlth_fault_test.cid = cmd.args.cid;
		op_args.hlth_fault_test.test_id = cmd.args.test_id;
		err = btmesh_perform_op(BTMESH_OP_HLTH_FAULT_TEST, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}
		
		shell_info(shell, "  Faults:");
		for (i = 0; i < op_args.hlth_fault_test.fault_count; i++) {
			shell_info(shell, "  - 0x%02x", op_args.hlth_fault_test.faults[i]);
		}
		shell_info(shell, "");

		break;

	case CMD_HLTH_PERIOD_GET:
		op_args.hlth_period_get.addr = cmd.args.addr;
		op_args.hlth_period_get.app_idx = cmd.args.app_idx;
		err = btmesh_perform_op(BTMESH_OP_HLTH_PERIOD_GET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Divisor: 0x%02x\n", op_args.hlth_period_get.divisor);
		break;

	case CMD_HLTH_PERIOD_SET:
		op_args.hlth_period_set.addr = cmd.args.addr;
		op_args.hlth_period_set.app_idx = cmd.args.app_idx;
		op_args.hlth_period_set.divisor = cmd.args.div;
		err = btmesh_perform_op(BTMESH_OP_HLTH_PERIOD_SET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			shell_info(shell, "  Health fast period divisor NOT set!\n");
			return -ENOEXEC;
		}

		if (op_args.hlth_period_set.divisor !=
				op_args.hlth_period_set.updated_divisor) {
			shell_info(shell, "  Health fast period divisor NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Health fast period divisor st\n");
		break;

	case CMD_HLTH_ATTN_GET:
		op_args.hlth_attn_get.addr = cmd.args.addr;
		op_args.hlth_attn_get.app_idx = cmd.args.app_idx;
		err = btmesh_perform_op(BTMESH_OP_HLTH_ATTN_GET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			return -ENOEXEC;
		}

		shell_info(shell, "  Attention timer: %ds\n", op_args.hlth_attn_get.attn);
		break;

	case CMD_HLTH_ATTN_SET:
		op_args.hlth_attn_set.addr = cmd.args.addr;
		op_args.hlth_attn_set.app_idx = cmd.args.app_idx;
		op_args.hlth_attn_set.attn = cmd.args.attn;
		err = btmesh_perform_op(BTMESH_OP_HLTH_ATTN_SET, &op_args);

		if (cmd_err_handler(shell, err, 0)) {
			shell_info(shell, "  Attention timer NOT set!\n");
			return -ENOEXEC;
		}

		if (op_args.hlth_attn_set.attn != op_args.hlth_attn_set.updated_attn) {
			shell_info(shell, "  Attention timer NOT set!\n");
			return -ENOEXEC;
		}

		shell_info(shell, "  Attention timer set\n");
		break;

	case CMD_HLTH_TIMEOUT_GET:
		btmesh_perform_op(BTMESH_OP_HLTH_TIMEOUT_GET, &op_args);

		shell_info(shell, "  Health client model timeout: %dms\n",
				op_args.hlth_timeout_get.timeout);
		break;

	case CMD_HLTH_TIMEOUT_SET:
		op_args.hlth_timeout_set.timeout = cmd.args.timeout;
		btmesh_perform_op(BTMESH_OP_HLTH_TIMEOUT_SET, &op_args);
		shell_info(shell, "  Health client model timeout set\n");
		break;
#endif // CONFIG_SHELL_MESH_HEALTH

	default:
		/* This should not be possible */
		return -ENOEXEC;
        }

        return 0;
}

void cli_msg_callback(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
        shell_print(shell,
                        "Recieved Mesh Model Message:\n"
                        "    - Opcode             : 0x%08x - %s\n"
                        "    - Network Index      : 0x%04x\n"
                        "    - Application Index  : 0x%04x\n"
                        "    - Source Address     : 0x%04x\n"
                        "    - Destination Address: 0x%04x\n"
                        "    - Payload:",
                        opcode, btmesh_parse_opcode(opcode), ctx->net_idx, ctx->app_idx, ctx->addr,
                        ctx->recv_dst);
        
        while(buf->len) {
                shell_print(shell, "    0x%02x", net_buf_simple_pull_u8(buf));
        }
}

void cli_hlth_cb(uint16_t addr, uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	int i;

	shell_print(shell,
			"Recieved Node Current Health Faults:\n"
			"    - Address   : 0x%04x\n"
			"    - Test ID   : 0x%04x\n"
			"    - Company ID: 0x%04x\n"
			"    - Faults",
			addr, test_id, cid);

	for (i = 0; i < fault_count; i++) {
		shell_print(shell, "          0x%02x", faults[i]);
	}

	if (i == 0) {
		shell_print(shell, "        None\n");
	} else {
		shell_print(shell, "");
	}
}

/* Forward declaration required */
static void dynamic_blocked_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_addr_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_subnet_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_app_key_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_addr_subnet_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_addr_app_key_func(size_t idx, struct shell_static_entry *entry);

SHELL_STATIC_SUBCMD_SET_CREATE(dummy_sub, SHELL_SUBCMD_SET_END);
SHELL_DYNAMIC_CMD_CREATE(dynamic_blocked_sub, dynamic_blocked_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_addr_sub, dynamic_addr_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_subnet_sub, dynamic_subnet_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_appkey_sub, dynamic_app_key_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_addr_subnet_sub, dynamic_addr_subnet_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_addr_app_key_sub, dynamic_addr_app_key_func);

static void dynamic_blocked_func(size_t idx, struct shell_static_entry *entry)
{
        entry->syntax = btmesh_get_blocked_beacon(idx);

        if (entry->syntax == NULL) {
                return;
        }

        entry->handler = NULL;
        entry->subcmd = &dummy_sub;
        entry->help = DYNAMIC_UNBLOCK_HELP;
}
static void dynamic_addr_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_addr(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dummy_sub;
        entry->help = DYNAMIC_ADDR_HELP;
}

static void dynamic_subnet_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dummy_sub;
        entry->help = DYNAMIC_SUBNET_VIEW_HELP;
}

static void dynamic_app_key_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_appkeys(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dummy_sub;
        entry->help = DYNAMIC_APPKEY_VIEW_HELP;
}

static void dynamic_addr_subnet_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_addr(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dynamic_subnet_sub;
        entry->help = DYNAMIC_ADDR_HELP;
}

static void dynamic_addr_app_key_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_addr(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dynamic_appkey_sub;
        entry->help = DYNAMIC_ADDR_HELP;
}

SHELL_STATIC_SUBCMD_SET_CREATE(node_feature_subs,
                SHELL_CMD(get, &dynamic_addr_sub, NODE_GET_HELP, NULL),
                SHELL_CMD(set, &dynamic_addr_sub, NODE_SET_HELP, NULL),
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(node_subnet_subs,
                SHELL_CMD(add, &dynamic_addr_subnet_sub, NODE_ADD_HELP, NULL),
                SHELL_CMD(get, &dynamic_addr_sub, NODE_GET_HELP, NULL),
                SHELL_CMD(delete, &dynamic_addr_subnet_sub, NODE_CMD_DELETE_HELP, NULL),
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(node_configured_subs,
                SHELL_CMD(set, &dynamic_addr_sub, NODE_SET_HELP, NULL),
                SHELL_CMD(unset, &dynamic_addr_sub, NODE_UNSET_HELP, NULL),
                SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(beacon_subs,
                SHELL_CMD_ARG(list, NULL, BEACON_LIST_HELP, cmd_handler, 1, 0),
                SHELL_CMD_ARG(block, NULL, BEACON_BLOCK_HELP, cmd_handler, 2, 0),
                SHELL_CMD_ARG(unblock, &dynamic_blocked_sub, BEACON_UNBLOCK_HELP, cmd_handler, 2,
			0), 
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(node_subs,
                SHELL_CMD_ARG(list, NULL, NODE_LIST_HELP, cmd_handler, 1, 0),
                SHELL_CMD_ARG(reset, &dynamic_addr_sub, NODE_RESET_HELP, cmd_handler, 2, 0),
                SHELL_CMD_ARG(discover, &dynamic_addr_sub, NODE_DISC_HELP, cmd_handler, 2, 0),
                SHELL_CMD_ARG(comp, &dynamic_addr_sub, NODE_COMP_HELP, cmd_handler, 2, 0),
                SHELL_CMD_ARG(beacon, &node_feature_subs, NODE_BEACON_HELP, cmd_handler, 3, 1),
                SHELL_CMD_ARG(ttl, &node_feature_subs, NODE_TTL_HELP, cmd_handler, 3, 1),
                SHELL_CMD_ARG(friend, &node_feature_subs, NODE_FRIEND_HELP, cmd_handler, 3, 1),
                SHELL_CMD_ARG(proxy, &node_feature_subs, NODE_PROXY_HELP, cmd_handler, 3, 1),
                SHELL_CMD_ARG(relay, &node_feature_subs, NODE_RELAY_HELP, cmd_handler, 3, 3),
#ifdef CONFIG_SHELL_MESH_HEARTBEAT
		SHELL_CMD_ARG(hb-sub-set, &dynamic_addr_sub, NODE_HB_SUB_HELP, cmd_handler, 3, 9),
		SHELL_CMD_ARG(hb-pub-set, &dynamic_addr_subnet_sub, NODE_HB_PUB_HELP, cmd_handler,
			3 ,5),
#endif // CONFIG_SHELL_MESH_HEARTBEAT
                SHELL_CMD_ARG(subnet, &node_subnet_subs, NODE_SUBNET_HELP, cmd_handler, 3, 1),
                SHELL_CMD_ARG(configured, &node_configured_subs, NODE_CONFIG_HELP, cmd_handler, 3,
			0),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(beacon, &beacon_subs, BEACON_HELP, NULL);
SHELL_CMD_REGISTER(node, &node_subs, NODE_HELP, NULL);

#if defined(CONFIG_SHELL_MESH_SIG) || defined(CONFIG_SHELL_MESH_VND)
SHELL_STATIC_SUBCMD_SET_CREATE(model_appkey_subs,
#ifdef CONFIG_SHELL_MESH_SIG
                SHELL_CMD(bind, &dynamic_addr_app_key_sub, MODEL_BIND_HELP, NULL),
                SHELL_CMD(unbind, &dynamic_addr_app_key_sub, MODEL_UNBIND_HELP, NULL),
                SHELL_CMD(get, &dynamic_addr_sub, MODEL_GET_HELP, NULL),
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
                SHELL_CMD(bind-vnd, &dynamic_addr_app_key_sub, MODEL_BIND_VND_HELP, NULL),
                SHELL_CMD(unbind-vnd, &dynamic_addr_app_key_sub, MODEL_UNBIND_VND_HELP, NULL),
                SHELL_CMD(get-vnd, &dynamic_addr_sub, MODEL_GET_VND_HELP, NULL),
#endif // CONFIG_SHELL_MESH_VND
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(model_pub_subs,
#ifdef CONFIG_SHELL_MESH_SIG
                SHELL_CMD(get, &dynamic_addr_sub, MODEL_GET_HELP, NULL),
                SHELL_CMD(set, &dynamic_addr_sub, MODEL_SET_HELP, NULL),
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
                SHELL_CMD(get-vnd, &dynamic_addr_sub, MODEL_GET_VND_HELP, NULL),
                SHELL_CMD(set-vnd, &dynamic_addr_sub, MODEL_SET_VND_HELP, NULL),
#endif // CONFIG_SHELL_MESH_VND
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(model_sub_subs,
#ifdef CONFIG_SHELL_MESH_SIG
                SHELL_CMD(add, &dynamic_addr_sub, MODEL_ADD_HELP, NULL),
                SHELL_CMD(delete, &dynamic_addr_sub, MODEL_DEL_HELP, NULL),
                SHELL_CMD(overwrite, &dynamic_addr_sub, MODEL_OVRW_HELP, NULL),
                SHELL_CMD(get, &dynamic_addr_sub, MODEL_GET_HELP, NULL),
#endif // CONFIG_SHELL_MESH_SIG
#ifdef CONFIG_SHELL_MESH_VND
                SHELL_CMD(add-vnd, &dynamic_addr_sub, MODEL_ADD_VND_HELP, NULL),
                SHELL_CMD(delete-vnd, &dynamic_addr_sub, MODEL_DEL_VND_HELP, NULL),
                SHELL_CMD(overwrite-vnd, &dynamic_addr_sub, MODEL_OVRW_VND_HELP, NULL),
                SHELL_CMD(get-vnd, &dynamic_addr_sub, MODEL_GET_VND_HELP, NULL),
#endif // CONFIG_SHELL_MESH_VND
                SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(model_subs,
                SHELL_CMD_ARG(appkey, &model_appkey_subs, MODEL_APP_KEY_HELP, cmd_handler, 5, 2),
                SHELL_CMD_ARG(publish, &model_pub_subs, MODEL_PUB_HELP, cmd_handler, 5, 9),
                SHELL_CMD_ARG(subscribe, &model_sub_subs, MODEL_SUB_HELP, cmd_handler, 5, 2),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(model, &model_subs, MODEL_HELP, NULL);
#endif // defined(CONFIG_SHELL_MESH_SIG) || defined(CONFIG_SHELL_MESH_VND)

#ifdef CONFIG_SHELL_MESH_MSG
static void dynamic_sub_addr_func(size_t idx, struct shell_static_entry *entry);
static void dynamic_subnet_app_key_func(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dynamic_sub_addr_sub, dynamic_sub_addr_func);
SHELL_DYNAMIC_CMD_CREATE(dynamic_subnet_app_key_sub, dynamic_subnet_app_key_func);

static void dynamic_sub_addr_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_sub_addr(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dummy_sub;
        entry->help = DYNAMIC_SUB_ADDR_HELP;
}

static void dynamic_subnet_app_key_func(size_t idx, struct shell_static_entry *entry)
{
        get_dynamic_subnets(idx, entry);
        entry->handler = NULL;
        entry->subcmd = &dynamic_appkey_sub;
        entry->help = DYNAMIC_SUBNET_VIEW_HELP;
}

SHELL_STATIC_SUBCMD_SET_CREATE(msg_subs,
                SHELL_CMD_ARG(subscribe, NULL, MSG_SUB_HELP, cmd_handler, 2, 0),
                SHELL_CMD_ARG(unsubscribe, &dynamic_sub_addr_sub, MSG_UNSUB_HELP, cmd_handler, 2,
			0),
                SHELL_CMD_ARG(send, &dynamic_subnet_app_key_sub, MSG_SEND_HELP, cmd_handler, 5, 1),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(message, &msg_subs, MSG_HELP, NULL);
#endif // CONFIG_SHELL_MESH_MSG

#ifdef CONFIG_SHELL_MESH_HEALTH
SHELL_STATIC_SUBCMD_SET_CREATE(fault_subs,
                SHELL_CMD(get, &dynamic_addr_app_key_sub, FAULT_GET_HELP, NULL),
                SHELL_CMD(clear, &dynamic_addr_app_key_sub, FAULT_CLEAR_HELP, NULL),
                SHELL_CMD(test, &dynamic_addr_app_key_sub, FAULT_TEST_HELP, NULL),
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(period_subs,
                SHELL_CMD(get, &dynamic_addr_app_key_sub, PERIOD_GET_HELP, NULL),
                SHELL_CMD(set, &dynamic_addr_app_key_sub, PERIOD_SET_HELP, NULL),
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(attn_subs,
                SHELL_CMD(get, &dynamic_addr_app_key_sub, ATTN_GET_HELP, NULL),
                SHELL_CMD(set, &dynamic_addr_app_key_sub, ATTN_SET_HELP, NULL),
                SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(timeout_subs,
                SHELL_CMD(get, NULL, TIMEOUT_GET_HELP, NULL),
                SHELL_CMD(set, NULL, TIMEOUT_SET_HELP, NULL),
                SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(hlth_subs,
                SHELL_CMD_ARG(fault, &fault_subs, HLTH_FAULT_HELP, cmd_handler, 5, 1),
                SHELL_CMD_ARG(period, &period_subs, HLTH_PERIOD_HELP, cmd_handler, 4, 1),
                SHELL_CMD_ARG(attention, &attn_subs, HLTH_ATTN_HELP, cmd_handler, 4, 1),
                SHELL_CMD_ARG(timeout, &timeout_subs, HLTH_TIMEOUT_HELP, cmd_handler, 2, 1),
                SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(health, &hlth_subs, HLTH_HELP, NULL);
#endif // CONFIG_SHELL_MESH_HEALTH


/******************************************************************************
 *  PUBLIC SHELL FUNCTIONS
 *****************************************************************************/
void cli_init(void)
{
        shell = shell_backend_uart_get_ptr();
}
