const std = @import("std");
const relay = @import("relay.zig");

pub const HttpSession = struct {
    arena: std.heap.ArenaAllocator,
    stream: std.net.Stream,

    pub fn init(arena: std.heap.ArenaAllocator, stream: std.net.Stream) HttpSession {
        return .{
            .stream = stream,
            .arena = arena,
        };
    }

    pub fn run(self: *HttpSession) !void {
        defer self.arena.deinit();
        defer {
            self.stream.close();
        }

        var stream = self.stream;

        var reader = stream.reader();

        var list = std.ArrayList([]u8).init(self.arena.allocator());
        defer list.deinit();

        while (true) {
            var buf: [128]u8 = undefined;
            const rsp = try reader.readUntilDelimiter(&buf, '\n');
            try list.append(rsp);
            if (rsp.len <= 1) {
                break;
            }
        }

        std.log.debug("lines size. {d} hostline {s}", .{ list.items.len, hostline });

        var parts = std.mem.split(u8, hostline, ":");

        var part_list = std.ArrayList([]const u8).init(self.arena.allocator());
        defer part_list.deinit();

        // 将拆分结果存入 ArrayList
        while (parts.next()) |part| {
            std.log.debug("part {s}", .{part});
            try part_list.append(part);
        }

        const domain: []const u8 = part_list.items[2];
        var port: u16 = 80;

        if (part_list.items.len == 4) {
            port = try std.fmt.parseInt(u16, part_list.items[3], 10);
        }

        std.log.debug("remote info {s}:{d}", .{ domain, port });

        const adds = try std.net.getAddressList(self.arena.allocator(), domain, port);
        defer adds.deinit();

        var remote = try std.net.tcpConnectToAddress(adds.addrs[0]);

        const rsp = "HTTP/1.1 200 Connection Established\r\n\r\n";

        try stream.writer().writeAll(rsp);

        try relay.copy_bidirectional(&stream, &remote);
    }
};
