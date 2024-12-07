const std = @import("std");
const relay = @import("relay.zig");

const heap = std.heap;
const net = std.net;
const time = std.time;

const VER: u8 = 0x05;
const V4: u8 = 0x01;
const DOMAIN: u8 = 0x03;
const V6: u8 = 0x04;
const SUC: u8 = 0x00;
const FAIL: u8 = 0x01;

const HTTP_SUC: []const u8 = "HTTP/1.1 200 Connection Established\r\n\r\n";
const HTTP_PORT: u16 = 80;

pub const Session = struct {
    arena: heap.ArenaAllocator,
    stream: net.Stream,
    laddr: ?net.Address,
    raddr: ?net.Address = null,
    start: time.Instant,

    pub fn init(arena: std.heap.ArenaAllocator, connection: net.Server.Connection, start: time.Instant) Session {
        return .{
            .arena = arena,
            .stream = connection.stream,
            .laddr = connection.address,
            .start = start,
        };
    }

    pub fn process(self: *Session) !void {
        defer self.arena.deinit();
        defer self.stream.close();

        var stream = self.stream;
        var buf: [1]u8 = undefined;
        _ = try stream.read(&buf);
        std.log.debug("incoming request. tid-{d}. type {any} {any}", .{
            std.Thread.getCurrentId(),
            buf,
            self.laddr,
        });

        if (buf[0] == VER) {
            return self.socks();
        }
        return self.http();
    }

    fn socks(self: *Session) !void {
        var stream = self.stream;
        var buf: [3]u8 = undefined;
        _ = try stream.read(&buf);
        std.log.debug("read frame. {any}", .{buf});

        const NO_AUTH = [2]u8{ 0x05, 0x00 };
        _ = try stream.write(&NO_AUTH);

        var request: [4]u8 = undefined;
        _ = try stream.read(&request);

        // assume v4
        var addr_buf: [4]u8 = undefined;
        _ = try stream.read(&addr_buf);

        var port_buf: [2]u8 = undefined;
        _ = try stream.read(&port_buf);

        const port = std.mem.readInt(u16, &port_buf, .big);
        self.raddr = std.net.Address.initIp4(addr_buf, port);

        std.log.debug("remote {any}", .{self.raddr});

        var remote = try std.net.tcpConnectToAddress(self.raddr.?);
        defer remote.close();

        const rpy: [10]u8 = .{ VER, SUC, SUC, V4, SUC, SUC, SUC, SUC, SUC, SUC };

        _ = try stream.write(&rpy);

        // 启动双向复制
        try relay.copy_bidirectional(
            &stream,
            &remote,
            self.laddr,
            self.raddr,
            self.start,
        );
    }

    fn http(self: *Session) !void {
        var stream = self.stream;

        var index: i32 = 0;

        var domain: []const u8 = "";
        var port: u16 = HTTP_PORT;

        while (true) {
            var buf: [128]u8 = undefined;
            const rsp = try stream.reader().readUntilDelimiter(&buf, '\n');

            if (index == 1) {
                const hostline = std.mem.trim(u8, rsp, "\r\n");
                var it = std.mem.splitScalar(u8, hostline, ':');

                var tmpidx: i32 = 0;
                while (it.next()) |line| {
                    if (tmpidx == 1) {
                        const linebuf = std.mem.trim(u8, line, " ");
                        const domain_buf = try self.arena.allocator().alloc(u8, linebuf.len);
                        std.mem.copyForwards(u8, domain_buf, linebuf);
                        domain = domain_buf;
                    } else if (tmpidx == 2) {
                        port = try std.fmt.parseInt(u16, line, 10);
                    }
                    tmpidx += 1;
                }
            }
            index += 1;
            if (rsp.len <= 1) {
                break;
            }
        }

        std.log.debug("remote info {s}:{d}", .{ domain, port });

        const adds = try std.net.getAddressList(self.arena.allocator(), domain, port);
        defer adds.deinit();

        self.raddr = adds.addrs[0];

        var remote = try std.net.tcpConnectToAddress(self.raddr.?);
        defer remote.close();

        try stream.writer().writeAll(HTTP_SUC);
        try relay.copy_bidirectional(
            &stream,
            &remote,
            self.laddr,
            self.raddr,
            self.start,
        );
    }
};
