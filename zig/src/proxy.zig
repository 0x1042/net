const std = @import("std");
// const socks = @import("socks.zig");
// const http = @import("http.zig");
const session = @import("session.zig");

const heap = std.heap;
const net = std.net;

pub var config = struct {
    host: []const u8 = "127.0.0.1",
    port: u16 = 5501,
}{};

pub fn run() !void {
    var gpa = heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(gpa.deinit() == .ok);
    const allocator = gpa.allocator();

    const address = try net.Address.resolveIp(config.host, config.port);

    var server = try address.listen(.{
        .reuse_address = true,
        .kernel_backlog = 1024,
    });

    defer server.deinit();

    std.log.info("server listening at auto://{any}", .{address});

    while (server.accept()) |conn| {
        try handle(allocator, conn);
    } else |err| {
        std.log.err("failed to accept connection {}", .{err});
    }
}

fn handle(allocator: std.mem.Allocator, connection: std.net.Server.Connection) !void {
    var arena = heap.ArenaAllocator.init(allocator);
    errdefer arena.deinit();

    const start = try std.time.Instant.now();

    const newsession = try arena.allocator().create(session.Session);
    newsession.* = session.Session.init(arena, connection, start);
    const thread = try std.Thread.spawn(.{}, session.Session.process, .{newsession});
    thread.detach();
}
