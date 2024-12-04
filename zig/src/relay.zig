const std = @import("std");

fn copy(from: *std.net.Stream, to: *std.net.Stream) !void {
    var buffer: [4096]u8 = undefined;
    while (true) {
        const bytes_read = try from.read(buffer[0..]);
        if (bytes_read == 0) break; // EOF
        _ = try to.writeAll(buffer[0..bytes_read]);
    }
}

pub fn copy_bidirectional(from: *std.net.Stream, to: *std.net.Stream) !void {
    const read = try std.Thread.spawn(.{}, copy, .{ from, to });
    const wrote = try std.Thread.spawn(.{}, copy, .{ to, from });
    read.join();
    wrote.join();
}
