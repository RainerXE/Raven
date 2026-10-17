const std = @import("std");
// (copies of bindings/zig of Raven: a build.zig only imports files of its project)
const xmlzig = @import("raven/xmlzig.zig");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const raven = b.createModule(.{ .root_source_file = b.path("raven/raven.zig"), .target = target, .optimize = optimize });
    // ravenc turns main.xmlzig into Zig: it is the program
    const exe = b.addExecutable(.{
        .name = "hello_zig",
        .root_module = b.createModule(.{ .root_source_file = xmlzig.compile(b, "main.xmlzig"), .target = target, .optimize = optimize }),
    });
    exe.root_module.addImport("raven", raven);
    exe.root_module.linkSystemLibrary("raven", .{});
    exe.root_module.link_libc = true;
    if (b.graph.env_map.get("RAVEN_LIB_DIR")) |dir| exe.root_module.addLibraryPath(.{ .cwd_relative = dir });
    b.installArtifact(exe);
    const run = b.addRunArtifact(exe);
    run.setCwd(b.path("."));
    b.step("run", "Run the program").dependOn(&run.step);
}
