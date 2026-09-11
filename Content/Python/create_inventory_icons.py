"""Render inventory portraits from the actual Driftstead meshes/materials.

Run in a dedicated UE 5.3 editor process (never with -NullRHI)::

    UnrealEditor-Cmd.exe Driftstead.uproject -unattended -nop4 -NoSplash \
        -d3d11 -DriftsteadIconCapture \
        -ExecutePythonScript=Content/Python/create_inventory_icons.py

This script uses an unsaved, empty capture world, leaves every gameplay map and
source mesh/material untouched, and keeps the editor alive across render ticks.
Existing generated Texture2D assets are updated in place, so references survive
re-runs. The opaque, dark-background portraits are intended for BLEND_Opaque
Canvas drawing, or an opaque UI material; scene-capture alpha is not a cutout.
"""
from __future__ import annotations

import json
import time
import traceback
from pathlib import Path

import unreal


DEST = "/Game/Driftstead/UI/Icons"
SIZE = 256
# Keep the same authored mesh assignment as ADriftItemActor::ApplyDefinitionVisuals.
# The three plain crate types are differentiated by the HUD's Chinese item name
# and rarity, exactly as their world models are; do not invent different goods.
ITEM_MESHES = {
    "Driftwood": "platform_planks",
    "Rope": "RopeCoil",
    "ScrapMetal": "tool_shovel",
    "Cloth": "flag",
    "SeedCrate": "crate",
    "FoodCrate": "crate_bottles",
    "SealedBarrel": "barrel",
    "MachineryCrate": "crate",
    "Electronics": "chest",
    "AnimalCrate": "crate",
}
ICON_PATHS = {item_id: f"{DEST}/T_Icon_{item_id}" for item_id in ITEM_MESHES}
_job = None


class InventoryIconCapture:
    def __init__(self):
        self.root = Path(unreal.Paths.project_dir())
        self.output = self.root / "Artifacts" / "InventoryIcons"
        self.output.mkdir(parents=True, exist_ok=True)
        self.report_path = self.root / "Artifacts" / "Logs" / "InventoryIcons.json"
        self.report_path.parent.mkdir(parents=True, exist_ok=True)
        self.report = {"passed": False, "size": SIZE, "icons": [], "errors": []}
        self.meshes = []
        self.entries = list(ITEM_MESHES.items())
        self.index = 0
        self.handle = None
        self.target = None
        self.world = None
        self.actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.actors = []
        self.started_at = time.monotonic()
        self.next_at = self.started_at
        self.phase = "setup"
        self.in_tick = False

    def _spawn(self, actor_class, location, rotation=unreal.Rotator()):
        actor = self.actor_subsystem.spawn_actor_from_class(
            actor_class, location, rotation, transient=True)
        if not actor:
            raise RuntimeError(f"Could not spawn capture actor: {actor_class}")
        self.actors.append(actor)
        return actor

    def setup(self):
        # A dedicated-process flag prevents silently discarding an artist's map.
        command_line = unreal.SystemLibrary.get_command_line().lower()
        if "-driftsteadiconcapture" not in command_line:
            raise RuntimeError("Launch a dedicated editor with -DriftsteadIconCapture; "
                               "this script must not replace an interactive editor's map.")
        if "-nullrhi" in command_line:
            raise RuntimeError("Inventory icons require rendering: use -d3d11, not -NullRHI.")

        unreal.EditorAssetLibrary.make_directory(DEST)
        # Preflight before changing the dedicated process's unsaved capture map.
        for item_id, mesh_name in self.entries:
            path = f"/Game/Driftstead/Art/S_{mesh_name}"
            mesh = unreal.load_asset(path)
            if not isinstance(mesh, unreal.StaticMesh):
                raise RuntimeError(f"Missing mesh for {item_id}: {path}")
            slots = mesh.get_editor_property("static_materials")
            if not slots or any(not slot.material_interface for slot in slots):
                raise RuntimeError(f"Mesh has no complete authored material assignment: {path}")
            self.meshes.append(mesh)

        self.world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        if not self.world:
            raise RuntimeError("Could not create the transient icon capture world")
        self.world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)
        unreal.SystemLibrary.execute_console_command(self.world, "r.Streaming.FullyLoadUsedTextures 1")

        for rotation, intensity, color in (
                (unreal.Rotator(-38, -35, 0), 3.6, unreal.LinearColor(1.0, .94, .86, 1.0)),
                (unreal.Rotator(-28, 145, 0), 1.8, unreal.LinearColor(.80, .90, 1.0, 1.0))):
            light = self._spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 300), rotation)
            component = light.get_component_by_class(unreal.DirectionalLightComponent)
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
            component.set_intensity(intensity)
            component.set_light_color(color)
            component.set_editor_property("cast_shadows", False)

        self.subject = self._spawn(unreal.StaticMeshActor, unreal.Vector())
        self.mesh_component = self.subject.get_component_by_class(unreal.StaticMeshComponent)
        self.mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        self.mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

        camera_location = unreal.Vector(180, -220, 155)
        camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, unreal.Vector())
        self.camera_right = unreal.MathLibrary.get_right_vector(camera_rotation)
        self.camera_up = unreal.MathLibrary.get_up_vector(camera_rotation)
        camera = self._spawn(unreal.SceneCapture2D, camera_location, camera_rotation)
        self.capture = camera.capture_component2d
        self.capture.projection_type = unreal.CameraProjectionMode.ORTHOGRAPHIC
        self.capture.ortho_width = 142.0
        self.capture.capture_every_frame = False
        self.capture.capture_on_movement = False
        self.capture.always_persist_rendering_state = True
        self.capture.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
        self.capture.primitive_render_mode = unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST
        # ShowOnlyActors is EditInstanceOnly and the Python wrapper rejects its
        # assignment on a capture component created by the editor actor factory.
        # The supported runtime function populates the component show-only list.
        self.capture.show_only_actor_components(self.subject)
        self.capture.show_flag_settings = [unreal.EngineShowFlagsSetting(name, False) for name in
                                          ("Atmosphere", "Fog", "MotionBlur", "Bloom", "TemporalAA")]
        settings = unreal.PostProcessSettings()
        settings.set_editor_property("override_auto_exposure_method", True)
        settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
        settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
        settings.set_editor_property("override_auto_exposure_bias", True)
        settings.set_editor_property("auto_exposure_bias", 1.0)
        self.capture.post_process_settings = settings
        self.capture.post_process_blend_weight = 1.0
        self.target = unreal.RenderingLibrary.create_render_target2d(
            self.world, SIZE, SIZE, unreal.TextureRenderTargetFormat.RTF_RGBA8,
            unreal.LinearColor(.012, .023, .030, 1.0), False)
        if not self.target:
            raise RuntimeError("Could not allocate icon render target; check the rendering backend")
        self.capture.texture_target = self.target
        self._set_subject()
        # Allow shaders, palette texture streaming, and the scene proxy to finish
        # on real editor frames. Blocking Python sleep would not render anything.
        self.next_at = time.monotonic() + 12.0
        self.phase = "capture"
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        unreal.log("[Driftstead] Inventory icon render capture started (10 portraits)")

    def _set_subject(self):
        mesh = self.meshes[self.index]
        box = mesh.get_bounding_box()
        center = (box.min + box.max) * .5
        size = box.max - box.min
        longest = max(size.x, size.y, size.z)
        if longest <= .001:
            raise RuntimeError(f"Invalid source bounds: {mesh.get_path_name()}")
        scale = 100.0 / longest
        self.mesh_component.set_static_mesh(mesh)
        # No override materials: use the exact shipped mesh palette/fiber shader.
        self.subject.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        self.subject.set_actor_location(center * -scale, False, True)
        # Fit the projected box, not only its longest world axis: a diagonally
        # viewed chest is wider than either X or Y and otherwise clips the frame.
        def projected_size(axis):
            return (abs(axis.x) * size.x + abs(axis.y) * size.y + abs(axis.z) * size.z) * scale
        self.capture.ortho_width = max(projected_size(self.camera_right),
                                      projected_size(self.camera_up)) * 1.22
        for slot in mesh.get_editor_property("static_materials"):
            slot.material_interface.set_force_mip_levels_to_be_resident(True, True, 20.0)

    def tick(self, delta_seconds):
        # Saving a Texture2D can pump Slate again (including source-control
        # notification dialogs). Re-entry would write a duplicate and skip the
        # following item when the outer callback advances its index.
        if self.in_tick:
            return
        self.in_tick = True
        try:
            now = time.monotonic()
            if now - self.started_at > 180.0:
                raise RuntimeError("Icon rendering exceeded the 180-second safety timeout")
            if now < self.next_at:
                return
            if self.phase == "capture":
                self.capture.capture_scene()
                self.phase = "write"
                self.next_at = now + .35
                return
            self._write_icon()
            self.index += 1
            if self.index == len(self.entries):
                if [row["item_id"] for row in self.report["icons"]] != list(ITEM_MESHES):
                    raise RuntimeError("Icon capture manifest contains missing or duplicate item IDs")
                self.report["passed"] = True
                self.finish()
                return
            self._set_subject()
            self.phase = "capture"
            self.next_at = now + .75
        except Exception:
            self.report["errors"].append(traceback.format_exc())
            unreal.log_error(self.report["errors"][-1])
            self.finish()
        finally:
            self.in_tick = False

    def _write_icon(self):
        item_id, mesh_name = self.entries[self.index]
        # Readback is also a render fence. A black/unrendered target must never be
        # silently baked into the shipped UI. Full visual inspection follows.
        samples = []
        for y in range(24, SIZE - 24, 26):
            for x in range(24, SIZE - 24, 26):
                pixel = unreal.RenderingLibrary.read_render_target_pixel(self.world, self.target, x, y)
                samples.append((pixel.r, pixel.g, pixel.b))
        unique_colors = len(set(samples))
        brightest = max(max(rgb) for rgb in samples)
        if unique_colors < 4 or brightest < 30:
            raise RuntimeError(f"Capture appears blank for {item_id}: "
                               f"{unique_colors} colors, max channel {brightest}")

        icon_path = ICON_PATHS[item_id]
        texture = unreal.load_asset(icon_path)
        if texture:
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError(f"Refusing to replace a non-Texture2D asset: {icon_path}")
            unreal.RenderingLibrary.convert_render_target_to_texture2d_editor_only(
                self.world, self.target, texture)
        else:
            texture = unreal.RenderingLibrary.render_target_create_static_texture2d_editor_only(
                self.target, icon_path, unreal.TextureCompressionSettings.TC_EDITOR_ICON,
                unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        if not texture:
            raise RuntimeError(f"Could not bake Texture2D asset: {icon_path}")
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save generated icon: {icon_path}")
        unreal.RenderingLibrary.export_render_target(self.world, self.target,
                                                   str(self.output), f"{item_id}.png")
        self.report["icons"].append({
            "item_id": item_id,
            "texture": icon_path,
            "mesh": f"/Game/Driftstead/Art/S_{mesh_name}",
            "preview": str(self.output / f"{item_id}.png"),
            "sample_color_count": unique_colors,
            "brightest_sample": brightest,
            "ortho_width": self.capture.ortho_width,
        })
        unreal.log(f"[Driftstead] Inventory icon saved: {icon_path}")

    def finish(self):
        if self.handle:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        if self.target:
            self.capture.texture_target = None
            unreal.RenderingLibrary.release_render_target2d(self.target)
            self.target = None
        for actor in reversed(self.actors):
            self.actor_subsystem.destroy_actor(actor)
        self.actors.clear()
        self.report_path.write_text(json.dumps(self.report, indent=2, ensure_ascii=False), encoding="utf-8")
        unreal.log(f"[Driftstead] Inventory icon capture complete: "
                   f"passed={self.report['passed']}, saved={len(self.report['icons'])}")
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)


def main():
    global _job
    if _job is not None and _job.handle:
        raise RuntimeError("Inventory icon capture is already running in this editor")
    _job = InventoryIconCapture()
    try:
        _job.setup()
    except Exception:
        _job.report["errors"].append(traceback.format_exc())
        _job.finish()
        raise


if __name__ == "__main__":
    main()
