import CppHeaderParser
import json
from components_common import FieldTypes

TYPE_MAP = {
    'float'             : FieldTypes.FIELD_FLOAT,
    'float2'            : FieldTypes.FIELD_FLOAT2,
    'float3'            : FieldTypes.FIELD_FLOAT3,
    'float4'            : FieldTypes.FIELD_FLOAT4,
    'string'            : FieldTypes.FIELD_STRING,
    'std::string'       : FieldTypes.FIELD_STRING,
    'int32_t'           : FieldTypes.FIELD_INT,
    'int'               : FieldTypes.FIELD_INT,
    'Handle<Mesh >'     : FieldTypes.FIELD_HANDLE_MESH,
    'Handle<Material >' : FieldTypes.FIELD_HANDLE_MAT,
    'Handle<Texture >'  : FieldTypes.FIELD_HANDLE_TEX,
    'uint32_t'          : FieldTypes.FIELD_UINT,
    'bool'              : FieldTypes.FIELD_BOOL,
    'uint64_t'          : FieldTypes.FIELD_UINT64,
    'int64_t'           : FieldTypes.FIELD_INT64,
}


def get_field_type(typeName, raw_type = None):
    if typeName in TYPE_MAP:
        return TYPE_MAP[typeName], typeName
    
    if raw_type in TYPE_MAP:
        typeName = raw_type
        return TYPE_MAP[raw_type], typeName

    return (FieldTypes.FIELD_UNDEFINED, typeName)


def generate_metadata(components: list):
    metadata = []
    processed_components = []
    for comp in components:
        comp_name = '{}::{}'.format(comp['namespace'], comp['name'])
        if comp_name in processed_components:
            continue

        properties = comp['properties']['public']
        comp_data = {
            'key': comp_name,
            'value':[]
        }
        
        if len(properties) > 0:
            for prop in properties:
                name = prop['name']
                type = prop['type']
                raw_type = prop['raw_type']
                field_type, type = get_field_type(type, raw_type)
                comp_data['value'].append({
                    'Name': name,
                    'Type': type,
                    'FieldTypeEnum': field_type
                })
                
        processed_components.append(comp_name)
        metadata.append(comp_data)

    return {"Components": metadata}

def process_file(file):
    components = []
    try:
        header = CppHeaderParser.CppHeader(file, str="file", encoding=None, preprocessed=True)
    except:
        print('Unable to parse "{}"'.format(file))
        return components

    if len(header.classes) > 0:
        for clsKey in header.classes:
            cls = header.classes[clsKey]
            if len(header.classes[clsKey]['inherits']) > 0:
                for base_cls in cls['inherits']:
                    if 'IComponent' in base_cls['class'] and 'IComponentPool' not in base_cls['class']:
                        components.append(cls)
            elif 'NV_COMPONENT' in clsKey:
                clsName = clsKey.replace('NV_COMPONENT', '')
                cls['name'] = clsName
                components.append(cls)
    return components

def parse_components(base_dir='../'):
    import glob
    import os

    components = []
    for file in glob.glob(base_dir + '**/*.h', recursive=True):
        if '.\\packages\\' in file or '.\\Shared\\' in file:
            continue
        components.extend(process_file(file))

    for file in glob.glob(base_dir + '**/*.cpp', recursive=True):
        if '.\\packages\\' in file:
            continue
        components.extend(process_file(file))

    return components


def main():
    components = parse_components('./')
    metadata = generate_metadata(components=components)
    meta_json = json.dumps({"Components": metadata}, indent=4)
    #print (meta_json)
    print('Writing metadata.json')
    with open("metadata.json", "w") as outfile:
        outfile.write(meta_json)

    import shutil
    shutil.copy2("metadata.json", ".\\x64\\Debug\\")
    shutil.copy2("metadata.json", ".\\x64\\Release\\")

if __name__ == '__main__':
    main()
