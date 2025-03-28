const std = @import("std");
const relay = @import("relay.zig");

const VER: u8 = 0x05;
const V4: u8 = 0x01;
const DOMAIN: u8 = 0x03;
const V6: u8 = 0x04;
const SUC: u8 = 0x00;
const FAIL: u8 = 0x01;

pub const SocksSession = struct {
    arena: std.heap.ArenaAllocator,
    stream: std.net.Stream,
    local_addr: ?std.net.Address,
    remote_addr: ?std.net.Address = null,

    pub fn init(arena: std.heap.ArenaAllocator, stream: std.net.Stream, addr: std.net.Address) SocksSession {
        return .{
            .stream = stream,
            .arena = arena,
            .local_addr = addr,
        };
    }

    pub fn run(self: *SocksSession) !void {
        defer self.arena.deinit();
        defer {
            self.stream.close();
        }

        const start = try std.time.Instant.now();

        var stream = self.stream;

        var buf: [3]u8 = undefined;
        _ = try stream.read(&buf);
        std.log.debug("read frame. {any}", .{buf});

        const suc: [2]u8 = .{ 0x05, 0x00 };
        _ = try stream.write(&suc);

        // read request

        var request: [4]u8 = undefined;
        _ = try stream.read(&request);

        // assume v4
        var addr_buf: [4]u8 = undefined;
        _ = try stream.read(&addr_buf);

        var port_buf: [2]u8 = undefined;
        _ = try stream.read(&port_buf);

        const port = std.mem.readInt(u16, &port_buf, .big);
        self.remote_addr = std.net.Address.initIp4(addr_buf, port);

        std.log.debug("remote {any}", .{self.remote_addr});

        var remote = try std.net.tcpConnectToAddress(self.remote_addr.?);
        defer remote.close();

        const rpy: [10]u8 = .{ VER, SUC, SUC, V4, SUC, SUC, SUC, SUC, SUC, SUC };

        _ = try stream.write(&rpy);

        // 启动双向复制
        try relay.copy_bidirectional(
            &stream,
            &remote,
            self.local_addr,
            self.remote_addr,
            start,
        );
    }
};
