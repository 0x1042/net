const std = @import("std");
const testing = std.testing;

const VER: u8 = 0x05;
const V4: u8 = 0x01;
const DOMAIN: u8 = 0x03;
const V6: u8 = 0x04;
const SUC: u8 = 0x00;
const FAIL: u8 = 0x01;

pub var config = struct {
    host: []const u8 = "127.0.0.1",
    port: u16 = 5501,
}{};

fn copy(from: *std.net.Stream, to: *std.net.Stream) !void {
    var buffer: [2048]u8 = undefined;
    while (true) {
        const bytes_read = try from.read(buffer[0..]);
        if (bytes_read == 0) break; // EOF
        _ = try to.writeAll(buffer[0..bytes_read]);
    }
}

pub fn copy_bidirectional(from: *std.net.Stream, to: *std.net.Stream) !void {
    const read = try std.Thread.spawn(.{}, copy, .{ from, to });
    read.detach();
    const wrote = try std.Thread.spawn(.{}, copy, .{ to, from });
    wrote.join();
}

const Session = struct {
    arena: std.heap.ArenaAllocator,
    stream: std.net.Stream,

    pub fn init(arena: std.heap.ArenaAllocator, stream: std.net.Stream) Session {
        return .{
            .stream = stream,
            .arena = arena,
        };
    }

    fn run(self: *Session) !void {
        defer self.arena.deinit();
        defer {
            self.stream.close();
        }

        var stream = self.stream;

        var buf: [4]u8 = undefined;
        _ = try stream.read(&buf);
        std.log.debug("read frame. {any}", .{buf});

        const suc: [2]u8 = .{ 0x05, 0x00 };
        _ = try stream.write(&suc);

        // read request

        var request: [4]u8 = undefined;
        _ = try stream.read(&request);

        // const adty = request[3];

        // assume v4
        var addr_buf: [4]u8 = undefined;
        _ = try stream.read(&addr_buf);

        var port_buf: [2]u8 = undefined;
        _ = try stream.read(&port_buf);

        const port = std.mem.readInt(u16, &port_buf, .big);

        const remote_addr = std.net.Address.initIp4(addr_buf, port);

        std.log.debug("remote_addr {any}", .{remote_addr});

        var remote = try std.net.tcpConnectToAddress(remote_addr);

        const rpy: [10]u8 = .{ VER, SUC, SUC, V4, SUC, SUC, SUC, SUC, SUC, SUC };

        _ = try stream.write(&rpy);

        // 启动双向复制
        try copy_bidirectional(&stream, &remote);
    }
};

pub fn run() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();

    const address = try std.net.Address.parseIp(config.host, config.port);
    var listener = try address.listen(.{
        .reuse_address = true,
        .reuse_port = true,
        .kernel_backlog = 1024,
        .force_nonblocking = true,
    });

    defer listener.deinit();
    std.log.info("listening at {any}\n", .{address});

    while (true) {
        if (listener.accept()) |conn| {
            var arena = std.heap.ArenaAllocator.init(allocator);
            const session = try arena.allocator().create(Session);
            errdefer arena.deinit();

            session.* = Session.init(arena, conn.stream);
            const thread = try std.Thread.spawn(.{}, Session.run, .{session});
            thread.detach();
            std.log.debug("incoming request {} \n", .{conn});
        } else |err| {
            std.log.err("failed to accept connection {}", .{err});
        }
    }
}
