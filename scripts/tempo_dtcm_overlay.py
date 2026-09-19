"""Place ACF working arrays in DTCM. MAC order unchanged. DualMCU files stay on disk."""
import hashlib

def apply_tempo_dtcm_overlay(root):
    path = root / 'core/audio/tempo_acf.cpp'
    before = path.read_text()
    after = before
    old = '    std::array<float, kTempoAcfHistoryLength> work{};'
    new = ('    static std::array<float, kTempoAcfHistoryLength> work '
           '__attribute__((section(".dtcm"), aligned(32)));\n'
           '    work = {};')
    if after.count(old) != 1:
        raise RuntimeError('tempo DTCM overlay work mismatch')
    after = after.replace(old, new)
    old = '    std::array<float, kAcfTableLength> acf{};'
    new = ('    static std::array<float, kAcfTableLength> acf '
           '__attribute__((section(".dtcm"), aligned(32)));\n'
           '    acf = {};')
    if after.count(old) != 1:
        raise RuntimeError('tempo DTCM overlay acf mismatch')
    after = after.replace(old, new)
    path.write_text(after)
    return {
        'core/audio/tempo_acf.cpp': {
            'before': hashlib.sha256(before.encode()).hexdigest(),
            'after': hashlib.sha256(after.encode()).hexdigest(),
        }
    }
