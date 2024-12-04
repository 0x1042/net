const std = @import("std");
const cli = @import("zig-cli");
const proxy = @import("proxy.zig");
const logger = @import("logger.zig");

pub const std_options = .{
    .log_level = .debug,
    .logFn = logger.logfn,
};

pub fn main() !void {
    var r = try cli.AppRunner.init(std.heap.page_allocator);
    const app = cli.App{
        .command = cli.Command{
            .name = "server",
            .options = &.{
                // Define an Option for the "host" command-line argument.
                .{
                    .long_name = "host",
                    .help = "host to listen on",
                    .value_ref = r.mkRef(&proxy.config.host),
                },

                // Define an Option for the "port" command-line argument.
                .{
                    .long_name = "port",
                    .help = "port to bind to",
                    .required = true,
                    .value_ref = r.mkRef(&proxy.config.port),
                },
            },
            .target = cli.CommandTarget{
                .action = cli.CommandAction{ .exec = proxy.run },
            },
        },
    };
    return r.run(&app);
}
