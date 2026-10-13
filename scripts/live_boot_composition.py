"""Validate complete authored v8 defaults and generate the existing boot consumer."""
import hashlib
import math
import struct

OUTPUT = {'hardware_output_brightness','emission_enabled','output_precision',
          'output_intensity_a','output_intensity_b','primary_intensity',
          'secondary_intensity','secondary_output_enabled'}
PAIR = {'pair_link','pair_colour_enabled','pair_colour_mode','pair_colour_strength',
        'pair_colour_spread','pair_colour_spatial'}

def fields_from_schema(schema):
    fields = {}
    def visit(value):
        if isinstance(value, dict):
            for name, field in value.get('fields', {}).items():
                if isinstance(field, dict) and 'id' in field and 'kind' in field:
                    fields[name] = field
            for child in value.values(): visit(child)
        elif isinstance(value, list):
            for child in value: visit(child)
    visit(schema)
    return {k:v for k,v in fields.items() if v.get('writable', True) and k != 'board'}

def validate_composition(defaults, schema, schema_bytes):
    if 'composition' not in defaults: return None
    c = defaults['composition']
    def exact(obj, keys, label):
        if not isinstance(obj, dict) or set(obj) != set(keys):
            raise RuntimeError(f'{label}: missing or unknown fields')
    exact(c, {'profile_version','schema_sha256','boards','pair','output'}, 'composition')
    if type(c['profile_version']) is not int or c['profile_version'] != 8:
        raise RuntimeError('composition requires profile_version 8')
    if c['schema_sha256'] != hashlib.sha256(schema_bytes).hexdigest():
        raise RuntimeError('composition schema identity mismatch')
    fields = fields_from_schema(schema)
    board_fields = set(fields)-OUTPUT-PAIR
    exact(c['boards'], {'0','1'}, 'boards')
    for label, values, names in [('board0',c['boards']['0'],board_fields),
                                 ('board1',c['boards']['1'],board_fields),
                                 ('pair',c['pair'],PAIR),('output',c['output'],OUTPUT|{'period_us'})]:
        exact(values,names,label)
        for name,value in values.items():
            if name=='period_us':
                if type(value) is not int or value != 10000: raise RuntimeError('donor boot period must be 10000 us')
                continue
            field=fields[name];kind=field['kind']
            if kind=='bool': valid=type(value) is bool
            elif kind in ('u32','i32'): valid=type(value) is int
            else: valid=type(value) in (float,int) and math.isfinite(value)
            if not valid: raise RuntimeError(f'{label}.{name}: wrong type or nonfinite')
            if kind!='bool':
                # Compare in the same F32 domain as the owning firmware setter.
                f32=lambda x: struct.unpack('<f',struct.pack('<f',x))[0]
                try: checked=f32(value) if kind=='f32' else value
                except OverflowError: raise RuntimeError(f'{label}.{name}: overflow')
                low=f32(field['min']) if kind=='f32' else field['min']
                high=f32(field['max']) if kind=='f32' else field['max']
                if not low<=checked<=high: raise RuntimeError(f'{label}.{name}: outside supported range')
                enum=field.get('enum')
                if enum and isinstance(enum[0],int) and value not in enum: raise RuntimeError(f'{label}.{name}: unsupported enum')
    # These scene fields are pair-owned by the existing scene setter.
    for name in ('scene_choreography','scene_initiator','scene_answer_delay_ms'):
        if c['boards']['0'][name] != c['boards']['1'][name]:
            raise RuntimeError(f'{name}: pair-owned values must agree on both boards')
    output=c['output']
    if output['hardware_output_brightness'] != defaults.get('brightness') or not 100<=output['hardware_output_brightness']<=255:
        raise RuntimeError('composition brightness must match explicit 100..255 live default')
    if not output['emission_enabled'] or defaults.get('emit_default')!='on':
        raise RuntimeError('composition must boot autonomous emission on')
    if any(c['boards'][str(i)]['current_mode']!=defaults.get(f'mode_{"ab"[i]}') for i in range(2)):
        raise RuntimeError('composition modes must match live defaults metadata')
    return c

def literal(value):
    if type(value) is bool: return 'true' if value else 'false'
    if type(value) is int: return str(value)
    return float(value).hex()+'f'

def render_composition(c):
    lines=['#pragma once','// Generated from hashed --live-defaults; no host/NVM dependency.',
           'template<class Palettes, class Audio> bool k1_apply_live_boot_composition(Palettes& palettes, Audio& audio, bool (*admit)()) {',
           '  static_assert(k1::titan::kPalettePeriodUs == 10000U, "boot composition period contract changed");',
           '  bool ok=true;']
    def add(x): lines.append('  '+x)
    for board in range(2):
        b=c['boards'][str(board)]; prefix='k1_secondary_emotiscope' if board else 'k1_emotiscope'
        add('{ K1EmotiscopeConfig cfg{}; '+prefix+'_get_config(&cfg);')
        for key in ('brightness','softness','warmth','speed','background','current_mode','mirror_mode','screensaver','temporal_dithering','auto_color_cycle','reverse_color_range'):
            add(f'cfg.{key}={literal(b[key])};')
        add(f'ok={prefix}_set_config(&cfg)&&ok; palettes.noteDonorMode(cfg.current_mode,{board}); }}')
        for setter,args in [('set_palette',['palette']),('set_response_gain',['response_gain']),('set_edge_response',['edge_response','edge_depth'])]:
            add(f'ok={prefix}_{setter}('+','.join(literal(b[x]) for x in args)+')&&ok;')
        for typ,setter,names in [
            ('K1EmotiscopeEdgeColourConfig','set_edge_colour',['edge_colour_enabled','edge_colour_mode','edge_colour_spatial','edge_colour_strength','edge_colour_spread']),
            ('K1EmotiscopeMusicalConfig','set_musical_config',['musical_role','musical_strength','musical_attack_ms','musical_release_ms']),
            ('K1EmotiscopeMlConfig','set_ml_config',['ml_enabled','ml_strength']),
            ('K1ComposedSceneConfig','set_scene_config',[x for x in b if x=='scene' or x.startswith('scene_')])]:
            # Scene structure ordering is explicit, independent of JSON key order.
            if typ=='K1ComposedSceneConfig': names=['scene']+['scene_'+x for x in ('outgoing_width','outgoing_ms','separation_ms','quiet_ms','return_width','return_ms','return_strength','trail_ms','trips','outgoing_feather','return_feather','recovery_ms','choreography','initiator','answer_delay_ms','phrase','ml_target','quiet_scope')]
            add('{ '+typ+' v{'+','.join(literal(b[x]) for x in names)+f'}}; ok={prefix}_{setter}(&v)&&ok; }}')
        add(f'ok=audio.setInhouseMood({board},{literal(b["inhouse_mood"])})&&ok;')
        add(f'ok=palettes.setInhouseParameters({board},'+','.join(literal(b[x]) for x in ('inhouse_sensitivity','inhouse_square_iter','inhouse_injection_photons'))+')&&ok;')
    p=c['pair'];o=c['output']
    add('K1EmotiscopeEdgePairConfig pair{'+literal(p['pair_link'])+',{'+','.join(literal(p[x]) for x in ('pair_colour_enabled','pair_colour_mode','pair_colour_spatial','pair_colour_strength','pair_colour_spread'))+'}};')
    add('ok=k1_emotiscope_set_edge_pair(&pair)&&ok;')
    add(f'palettes.setHardwareBrightness({o["hardware_output_brightness"]});')
    add(f'palettes.setOutputPrecision({literal(o["output_precision"])});')
    for i,key in enumerate(('output_intensity_a','output_intensity_b')): add(f'ok=palettes.setOutputIntensity({i},{literal(o[key])})&&ok;')
    for i,key in enumerate(('primary_intensity','secondary_intensity')): add(f'ok=palettes.setBoardIntensity({i},{literal(o[key])})&&ok;')
    add(f'const bool top={literal(o["secondary_output_enabled"])};')
    add('const bool admitted=!top || admit(); palettes.setSecondaryOutputEnabled(top && admitted); ok=admitted&&ok;')
    add('palettes.setEmissionEnabled(true); return ok; }')
    return '\n'.join(lines)+'\n'
