#include <stdio.h>
#include <string.h>
#include "../../core/cli.h"
#include "syn_scan.h"
#include "udp_scan.h"
#include "banner.h"
#include "host_discovery.h"

int main(int argc, char *argv[])
{
    cli_args args = parse_cli(argc, argv);

    if (strcmp(args.module, "syn") == 0) {
        syn_scan(&args);
    }
    // else if (strcmp(args.module, "udp") == 0) {
    //     udp_scan(&args);
    // }
    // else if (strcmp(args.module, "banner") == 0) {
    //     banner_scan(&args);
    // }
    // else if (strcmp(args.module, "discover") == 0) {
    //     host_discovery(&args);
    // }
    else {
        printf("未知模块：%s\n", args.module);
    }

    return 0;
}
