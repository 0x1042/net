const std = @import("std");
const socks = @import("socks.zig");
const http = @import("http.zig");

pub var config = struct {
    host: []const u8 = "127.0.0.1",
    port: u16 = 5501,
}{};

pub fn run() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(gpa.deinit() == .ok);
    const allocator = gpa.allocator();

    const address = try std.net.Address.parseIp(config.host, config.port);

    var server = try address.listen(.{
        .reuse_address = true,
        .reuse_port = true,
        .kernel_backlog = 1024,
    });

    defer server.deinit();

    std.log.info("listening at {any}", .{address});

    while (true) {
        if (server.accept()) |conn| {
            var arena = std.heap.ArenaAllocator.init(allocator);

            var buf: [1]u8 = undefined;
            _ = try conn.stream.read(&buf);
            std.log.debug("incoming request. type {any} {}", .{ buf, conn });

            const ftype: u8 = buf[0];

            if (ftype == 0x05) {
                const session = try arena.allocator().create(socks.SocksSession);
                errdefer arena.deinit();
                session.* = socks.SocksSession.init(arena, conn.stream);
                const thread = try std.Thread.spawn(.{}, socks.SocksSession.run, .{session});
                thread.detach();
            } else {
                const session = try arena.allocator().create(http.HttpSession);
                errdefer arena.deinit();
                session.* = http.HttpSession.init(arena, conn.stream);
                const thread = try std.Thread.spawn(.{}, http.HttpSession.run, .{session});
                thread.detach();
            }
        } else |err| {
            std.log.err("failed to accept connection {}", .{err});
        }
    }
}
