import CppHeaderParser
import json
from components_common import FieldTypes

TYPE_MAP = {
    'float': FieldTypes.FIELD_FLOAT,
    'float2': FieldTypes.FIELD_FLOAT2,
    'float3': FieldTypes.FIELD_FLOAT3,
    'float4': FieldTypes.FIELD_FLOAT4,
    'string': FieldTypes.FIELD_STRING,
    'int32_t': FieldTypes.FIELD_INT
}


def get_field_type(typeName):
    if typeName in TYPE_MAP:
        return TYPE_MAP[typeName]

    return FieldTypes.FIELD_UNDEFINED


def generate_metadata(components: list):
    metadata = []
    for comp in components:
        comp_name = '{}::{}'.format(comp['namespace'], comp['name'])
        properties = comp['properties']['public']
        comp_data = {
            'key': comp_name,
            'value':[]
        }
        
        if len(properties) > 0:
            for prop in properties:
                name = prop['name']
                type = prop['type']
                field_type = get_field_type(type)
                comp_data['value'].append({
                    'Name': name,
                    'Type': type,
                    'FieldTypeEnum': field_type
                })

        metadata.append(comp_data)

    return {"Components": metadata}


def parse_components(base_dir='../'):
    import glob
    import os

    components = []

    # glob.glob() return a list of file name with specified pathname
    for file in glob.glob(base_dir + '**/*.h', recursive=True):
        if '.\\packages\\' in file or '.\\Shared\\' in file:
            continue

        try:
            header = CppHeaderParser.CppHeader(file)
        except:
            print('Unable to parse "{}"'.format(file))
            continue

        if len(header.classes) > 0:
            for clsKey in header.classes:
                if len(header.classes[clsKey]['inherits']) > 0:
                    cls = header.classes[clsKey]
                    for base_cls in cls['inherits']:
                        if 'IComponent' in base_cls['class'] and 'IComponentPool' not in base_cls['class']:
                            components.append(cls)

    return components


def main():
    components = parse_components('./')
    metadata = generate_metadata(components=components)
    meta_json = json.dumps({"Components": metadata}, indent=4)
    with open("metadata.json", "w") as outfile:
        outfile.write(meta_json)


if __name__ == '__main__':
    main()
