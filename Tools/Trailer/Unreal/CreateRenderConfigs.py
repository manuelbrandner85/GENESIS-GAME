# GENESIS Trailer - legt Movie-Render-Queue-Konfigurationen an (idempotent).
#   MRQ_Test_1080p     : 1920x1080 PNG, 1 Spatial / 8 Temporal Samples - Schnitt- und Look-Pruefung
#   MRQ_Master_UHD_EXR : 3840x2160 EXR 16 Bit (linear), 2 Spatial / 16 Temporal Samples, Motion Blur aus Temporal Samples
# Ausgabe der Renders: Genesis/Saved/MovieRenders/<Sequenz>/<Preset>/<Sequenz>.<Frame>.png|exr
#
# Aufruf: powershell -ExecutionPolicy Bypass -File Tools\Trailer\Run-UnrealPython.ps1 -Script Tools\Trailer\Unreal\CreateRenderConfigs.py

import unreal

FOLDER = "/Game/Genesis/Trailer/Render"
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
failures = []

# Qualitaets-CVars nur fuer den Render (nicht global): Lumen/Schatten/Reflexionen fuer Cinematics
QUALITY_CVARS = {
    "r.ScreenPercentage": 100.0,
    "r.Lumen.ScreenProbeGather.ScreenTraces.HZBTraversal.FullResDepth": 1.0,
    "r.Lumen.ScreenProbeGather.DownsampleFactor": 8.0,
    "r.Lumen.DiffuseIndirect.Allow": 1.0,
    "r.Lumen.Reflections.DownsampleFactor": 1.0,
    "r.Shadow.Virtual.ResolutionLodBiasDirectional": -1.5,
    "r.Shadow.Virtual.ResolutionLodBiasLocal": -1.5,
    "r.VolumetricFog.GridPixelSize": 4.0,
    "r.VolumetricFog.GridSizeZ": 128.0,
    "r.VolumetricFog.HistoryMissSupersampleCount": 16.0,
    "r.DepthOfFieldQuality": 4.0,
    "r.MotionBlurQuality": 4.0,
    "r.Tonemapper.Sharpen": 0.0,
}


def make_config(name, width, height, exr, spatial, temporal):
    path = FOLDER + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    if not eal.does_directory_exist(FOLDER):
        eal.make_directory(FOLDER)
    factory = None
    for cls_name in ("MoviePipelinePrimaryConfigFactory", "MoviePipelineConfigFactory"):
        if hasattr(unreal, cls_name):
            factory = getattr(unreal, cls_name)()
            break
    cfg = tools.create_asset(name, FOLDER, unreal.MoviePipelinePrimaryConfig, factory)
    if cfg is None:
        failures.append("Konfiguration nicht erstellbar: " + name)
        return None

    out = cfg.find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    out.set_editor_property("output_directory", unreal.DirectoryPath("{project_dir}/Saved/MovieRenders/{sequence_name}/" + name))
    out.set_editor_property("file_name_format", "{sequence_name}.{frame_number}")
    out.set_editor_property("output_resolution", unreal.IntPoint(width, height))
    out.set_editor_property("zero_pad_frame_numbers", 5)
    out.set_editor_property("override_existing_output", True)

    cfg.find_or_add_setting_by_class(unreal.MoviePipelineDeferredPassBase)
    if exr:
        exr_setting = cfg.find_or_add_setting_by_class(unreal.MoviePipelineImageSequenceOutput_EXR)
        exr_setting.set_editor_property("compression", unreal.EXRCompressionFormat.PIZ)
    else:
        cfg.find_or_add_setting_by_class(unreal.MoviePipelineImageSequenceOutput_PNG)

    aa = cfg.find_or_add_setting_by_class(unreal.MoviePipelineAntiAliasingSetting)
    aa.set_editor_property("spatial_sample_count", spatial)
    aa.set_editor_property("temporal_sample_count", temporal)
    aa.set_editor_property("override_anti_aliasing", True)
    aa.set_editor_property("anti_aliasing_method", unreal.AntiAliasingMethod.AAM_NONE)
    aa.set_editor_property("use_camera_cut_for_warm_up", True)
    aa.set_editor_property("engine_warm_up_count", 48)
    aa.set_editor_property("render_warm_up_count", 48)
    aa.set_editor_property("render_warm_up_frames", True)

    game = cfg.find_or_add_setting_by_class(unreal.MoviePipelineGameOverrideSetting)
    game.set_editor_property("cinematic_quality_settings", True)
    game.set_editor_property("texture_streaming", unreal.MoviePipelineTextureStreamingMethod.DISABLED)

    cv = cfg.find_or_add_setting_by_class(unreal.MoviePipelineConsoleVariableSetting)
    cvars = {}
    for k, v in QUALITY_CVARS.items():
        cvars[k] = v
    try:
        cv.set_editor_property("console_variables", cvars)
    except Exception:
        for k, v in cvars.items():
            cv.add_or_update_console_variable(k, v)
    eal.save_loaded_asset(cfg, only_if_is_dirty=False)
    unreal.log("GENESIS_TRAILER Render-Konfiguration {} {}x{} {} S{}/T{}".format(name, width, height, "EXR" if exr else "PNG", spatial, temporal))
    return cfg


make_config("MRQ_Test_1080p", 1920, 1080, False, 1, 8)
make_config("MRQ_Master_UHD_EXR", 3840, 2160, True, 2, 16)
if failures:
    for f in failures:
        unreal.log_error("GENESIS_TRAILER_FAIL " + f)
else:
    unreal.log("GENESIS_TRAILER_OK")
