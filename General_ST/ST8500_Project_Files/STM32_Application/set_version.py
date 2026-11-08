if __name__ == '__main__':
    import sys
    import versioning

    if len(sys.argv) >= 3:
        versioning.create_version(sys.argv[1], sys.argv[2])
    else:
        print("Missing argument: use 'python set_version.py COORD|DEVICE NUCLEO-G070RB|NUCLEO-G474RE|NUCLEO-L476RG'")
