const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .x86_64, .os_tag = .linux },
    .{ .cpu_arch = .aarch64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .linux },
};

fn collect(b: *std.Build) !std.ArrayList([]u8) {
    var srcs = std.ArrayList([]u8).init(b.allocator);
    var dir = try std.fs.cwd().openDir(".", .{
        .access_sub_paths = true,
        .iterate = true,
    });
    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        const ext = std.fs.path.extension(entry.basename);

        if (std.mem.eql(u8, ".c", ext)) {
            try srcs.append(b.dupe(entry.path));
        }
    }
    return srcs;
}

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});

    const flags = .{
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror=return-type",
        "-std=gnu23",
        "-Wformat",
        "-Werror=format",
    };

    const cfiles = try collect(b);
    defer cfiles.deinit();

    for (targets) |t| {
        var name: []u8 = undefined;
        const arch = if (t.cpu_arch) |arch| @tagName(arch) else "native";
        const os = if (t.os_tag) |os| @tagName(os) else "unknown";
        if (t.abi) |abi| {
            name = b.fmt("{s}-{s}-{s}", .{ os, arch, @tagName(abi) });
        } else {
            name = b.fmt("{s}-{s}", .{ os, arch });
        }

        const exe = b.addExecutable(.{
            .name = name,
            .target = b.resolveTargetQuery(t),
            .optimize = optimize,
        });

        exe.addCSourceFiles(.{
            .files = cfiles.items,
            .flags = &flags,
        });

        exe.linkLibC();
        b.installArtifact(exe);
    }
}
