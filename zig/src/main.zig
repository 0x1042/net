const std = @import("std");
const cli = @import("zig-cli");
const proxy = @import("proxy.zig");
const logger = @import("logger.zig");
const config = @import("config");

pub const std_options = .{
    .log_level = .debug,
    .logFn = logger.logfn,
};

pub fn main() !void {
    var runner = try cli.AppRunner.init(std.heap.page_allocator);

    const action = cli.CommandAction{
        .exec = proxy.run,
    };

    const version = config.date ++ " " ++ config.version;

    const host_opt = cli.Option{
        .long_name = "host",
        .short_alias = 'l',
        .help = "host to listen on",
        .value_ref = runner.mkRef(&proxy.config.host),
    };

    const port_opt = cli.Option{
        .long_name = "port",
        .short_alias = 'p',
        .help = "port to bind to",
        .value_ref = runner.mkRef(&proxy.config.port),
    };

    const cmd = cli.Command{
        .name = "server",
        .options = &.{
            host_opt,
            port_opt,
        },
        .target = cli.CommandTarget{
            .action = action,
        },
    };

    const app = cli.App{
        .command = cmd,
        .version = version,
    };
    return runner.run(&app);
}
