import tifffile
import subprocess
import os

def extract_all_tags(tiff_path):

    tags_per_page = []
    with tifffile.TiffFile(tiff_path) as tif:
        for page in tif.pages:
            tag_dict = {tag.name: tag.value for tag in page.tags.values()}
            tags_per_page.append(tag_dict)
    return tags_per_page

def write_tags_to_txt(tags_per_page, output_path):

    with open(output_path, 'w', encoding='utf-8') as f:
        for page_index, tags in enumerate(tags_per_page):
            f.write(f"--- Page {page_index} ---\n")
            for name, value in sorted(tags.items()):
                # Detect any tag whose name contains 'Offset' and whose value is a sequence
                if 'Offset' in name and isinstance(value, (list, tuple)):
                    # join all offsets into one comma-separated line
                    offsets = ", ".join(str(v) for v in value)
                    f.write(f"{name}: [{offsets}]\n")
                else:
                    # default formatting
                    f.write(f"{name}: {value!r}\n")
            f.write("\n")

if __name__ == "__main__":
    tiff_path = r"C:\Projects\CZIConvert\out\build\x64-Debug\output.svs"
    output_txt = r"C:\Projects\CZIConvert\out\build\x64-Debug\output.txt"
    all_tags = extract_all_tags(tiff_path)
    write_tags_to_txt(all_tags, output_txt)
    print(f"Wrote {len(all_tags)} pages of tags to {output_txt}")
    subprocess.Popen([(r"C:\Program Files\Notepad++\notepad++.exe"), output_txt], shell=True)