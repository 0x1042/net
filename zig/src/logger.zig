const std = @import("std");
const Chameleon = @import("chameleon");
const time = @import("time.zig");

// custom logging function
pub fn logfn(
    comptime message_level: std.log.Level,
    comptime scope: @TypeOf(.EnumLiteral),
    comptime format: []const u8,
    args: anytype,
) void {
    comptime var ct = Chameleon.initComptime();
    const level = comptime switch (message_level) {
        .warn => ct.yellow().fmt("WARN "),
        .info => ct.blue().fmt("INFO "),
        .debug => ct.grey().fmt("DEBUG"),
        .err => ct.red().fmt("ERROR"),
    };

    const prefix = if (scope == .default) " " else "(" ++ @tagName(scope) ++ ") ";
    const stderr = std.io.getStdErr().writer();
    var bw = std.io.bufferedWriter(stderr);
    const writer = bw.writer();

    std.debug.lockStdErr();
    defer std.debug.unlockStdErr();
    nosuspend {
        // 2024-09-23T08:27:16Z
        time.DateTime.now().format("YYY-MM-DDTHH:mm:ssZ", .{}, writer) catch return;
        writer.print(" " ++ level ++ prefix ++ format ++ "\n", args) catch return;
        bw.flush() catch return;
    }
}
