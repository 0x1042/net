const std = @import("std");
const Thread = std.Thread;

fn io_copy(reader: anytype, writer: anytype, total: *usize) !void {
    var buffer: [1024 * 32]u8 = undefined;
    while (true) {
        const len = try reader.read(buffer[0..]);
        if (len == 0) {
            break;
        }
        _ = try writer.writeAll(buffer[0..len]);
        total.* += len;
    }
}

fn copy(from: *std.net.Stream, to: *std.net.Stream, total: *usize) !void {
    return io_copy(from.reader(), to.writer(), total);
}

pub fn copy_bidirectional(
    from: *std.net.Stream,
    to: *std.net.Stream,
    local_addr: ?std.net.Address,
    remote_addr: ?std.net.Address,
    start: std.time.Instant,
) !void {
    var rsize: usize = 0;
    var wsize: usize = 0;

    const read = try Thread.spawn(.{}, copy, .{
        from,
        to,
        &rsize,
    });
    const wrote = try Thread.spawn(.{}, copy, .{
        to,
        from,
        &wsize,
    });
    read.join();
    wrote.join();

    const end = try std.time.Instant.now();

    std.log.info("{any} -> {any}, read {d} bytes write {d} bytes, cost {d} ms.", .{
        local_addr,
        remote_addr,
        rsize,
        wsize,
        end.since(start) / 1000 / 1000,
    });
}
