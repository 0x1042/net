const std = @import("std");
const relay = @import("relay.zig");

const SUC: []const u8 = "HTTP/1.1 200 Connection Established\r\n\r\n";
const HTTP_PORT: u16 = 80;

pub const HttpSession = struct {
    arena: std.heap.ArenaAllocator,
    stream: std.net.Stream,
    local_addr: ?std.net.Address,
    remote_addr: ?std.net.Address = null,

    pub fn init(arena: std.heap.ArenaAllocator, stream: std.net.Stream, addr: std.net.Address) HttpSession {
        return .{
            .stream = stream,
            .arena = arena,
            .local_addr = addr,
        };
    }

    pub fn run(self: *HttpSession) !void {
        defer self.arena.deinit();
        defer {
            self.stream.close();
        }
        const start = try std.time.Instant.now();

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

        self.remote_addr = adds.addrs[0];

        var remote = try std.net.tcpConnectToAddress(self.remote_addr.?);
        defer remote.close();

        try stream.writer().writeAll(SUC);
        try relay.copy_bidirectional(
            &stream,
            &remote,
            self.local_addr,
            self.remote_addr,
            start,
        );
    }
};
