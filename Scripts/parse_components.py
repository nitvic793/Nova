import CppHeaderParser

header = CppHeaderParser.CppHeader("D:\\Personal\\Nova\\Renderer\\Components\\Renderable.h")

print(header)

def parse_components(base_dir = '../'):
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
    print(components)

if __name__=='__main__':
    main()

