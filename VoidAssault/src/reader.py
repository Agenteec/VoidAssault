import os

def merge_files(root_dir, output_file, extensions=None):
    """
    Рекурсивно считывает все файлы из root_dir и записывает в один файл.
    Перед каждым файлом вставляется комментарий с его путём.
    extensions — список расширений, если нужно фильтровать (например ['.h', '.cpp'])
    Если extensions=None — берутся все файлы.
    """
    with open(output_file, "w", encoding="utf-8") as out:
        for folder, _, files in os.walk(root_dir):
            for file in files:
                file_path = os.path.join(folder, file)
                rel_path = os.path.relpath(file_path, root_dir)
                
                # Фильтр по расширениям, если нужно
                if extensions is not None:
                    _, ext = os.path.splitext(file)
                    if ext.lower() not in extensions:
                        continue

                out.write(f"\n// {rel_path}\n")

                try:
                    with open(file_path, "r", encoding="utf-8") as f:
                        out.write(f.read())
                except Exception as e:
                    out.write(f"[Ошибка при чтении файла: {e}]\n")


if __name__ == "__main__":
    merge_files(
        root_dir=".", 
        output_file="merged.txt",
        extensions=None 
    )
