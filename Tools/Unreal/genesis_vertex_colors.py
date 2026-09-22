# GENESIS – Vertexfarben eines Static Mesh messen (Unreal-Python).
#
# Warum es diese Datei gibt (gemessen am 2026-09-22, UE 5.8, GENESIS-040 Teil 3):
#
#   `unreal.EditorStaticMeshLibrary.has_vertex_colors(mesh)` antwortet im Kommandlet
#   (`UnrealEditor-Cmd.exe -run=pythonscript`) IMMER mit False – unabhängig davon, ob das Mesh Farben hat.
#   Dasselbe gilt für die Geschwister derselben (seit UE 5.0 veralteten) Bibliothek:
#   `get_number_verts` und `get_num_uv_channels` liefern 0 für einen Würfel mit 8 Vertices und UV-Kanal.
#   Diese Funktionen lesen Renderdaten, die ein Kommandlet gar nicht aufbaut.
#
#   Die Fehldiagnose daraus („Vertexfarben kommen in Unreal nicht an", Docs/30) hat den Import verdächtigt,
#   obwohl er sauber arbeitet: Die Eileiterwand kommt mit R 0,004…0,996 (M 0,612), G 0,000…1,000 (M 0,332),
#   B 0,000…0,996 (M 0,512) an – dieselben Werte wie in der FBX. Mit und ohne Nanite gleich.
#
# Gemessen wird hier deshalb die Quelle selbst (MeshDescription) über Geometry Script. Dafür ist das
# Plugin `GeometryScripting` im Projekt eingeschaltet (nur für den Editor, nicht im gebauten Spiel).

import unreal


def measure_vertex_colors(static_mesh, lod=0, samples=4000):
    """Liest die Vertexfarben aus den Quelldaten (MeshDescription) und gibt eine Kennzahl je Kanal zurück.

    Rückgabe: dict mit 'count' und je Kanal (r, g, b, a) ein Tupel (min, max, Mittelwert),
    oder None, wenn Geometry Script nicht verfügbar ist.
    """
    if not static_mesh:
        return None
    if not hasattr(unreal, "GeometryScript_AssetUtils"):
        unreal.log_warning("GENESIS: Geometry Script fehlt – Vertexfarben nicht messbar "
                           "(Plugin 'GeometryScripting' im Projekt einschalten).")
        return None

    mesh = unreal.DynamicMesh()
    mesh = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        static_mesh, mesh, unreal.GeometryScriptCopyMeshFromAssetOptions(),
        unreal.GeometryScriptMeshReadLOD(unreal.GeometryScriptLODType.SOURCE_MODEL, lod))[0]
    colors = unreal.GeometryScript_VertexColors.get_mesh_per_vertex_colors(mesh)[1]
    count = unreal.GeometryScript_List.get_color_list_length(colors)
    if count <= 0:
        return {"count": 0}

    step = max(count // max(samples, 1), 1)
    channels = {"r": [], "g": [], "b": [], "a": []}
    for index in range(0, count, step):
        color = unreal.GeometryScript_List.get_color_list_item(colors, index)
        color = color[0] if isinstance(color, tuple) else color
        channels["r"].append(color.r)
        channels["g"].append(color.g)
        channels["b"].append(color.b)
        channels["a"].append(color.a)

    result = {"count": count}
    for name, values in channels.items():
        result[name] = (min(values), max(values), sum(values) / len(values))
    return result


def log_vertex_colors(label, static_mesh, expect=("r", "g", "b")):
    """Misst und protokolliert; meldet einen Fehler, wenn ein erwarteter Kanal überall gleich ist.

    Ein Kanal ohne Spannweite trägt keine Information – genau das wäre der Schaden, den ein verlorener
    Vertexfarben-Import anrichtet. Ein konstanter Kanal ist also der Befund, auf den es ankommt,
    nicht die Frage „gibt es überhaupt Farben".
    """
    data = measure_vertex_colors(static_mesh)
    if data is None:
        return None
    if not data.get("count"):
        unreal.log_error("GENESIS: %s hat keine Vertexfarben." % label)
        return data

    parts = []
    flat = []
    for name in ("r", "g", "b", "a"):
        low, high, mean = data[name]
        parts.append("%s %.3f…%.3f (M %.3f)" % (name.upper(), low, high, mean))
        if name in expect and high - low < 0.01:
            flat.append(name.upper())
    unreal.log("GENESIS: %s Vertexfarben, %d Vertices: %s" % (label, data["count"], " | ".join(parts)))
    if flat:
        unreal.log_error("GENESIS: %s – Kanal %s ohne Spannweite, das Material rechnet damit ins Leere."
                         % (label, ", ".join(flat)))
    return data
