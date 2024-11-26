import os
import yaml


def load_yaml_file(file_path):
    try:
        with open(file_path, 'r') as file:
            content = yaml.safe_load(file)
        return content
    except yaml.YAMLError as e:
        print(f"Error parsing YAML file: {e}")
        return None
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return None


def create_empty_directory(directory_path):
    # Check if the directory exists
    if os.path.exists(directory_path):
        # If it exists, remove all contents (files and subdirectories)
        for root, _, files in os.walk(directory_path, topdown=False):
            for file in files:
                os.remove(os.path.join(root, file))

    # If the directory doesn't exist, create it
    else:
        os.makedirs(directory_path)


def create_full_path(path_suffix, file_path):
    abs_file_path = ""
    if os.path.isabs(file_path):
        abs_file_path = file_path
    else:
        abs_file_path = os.path.join(path_suffix, file_path)

    if not os.path.exists(abs_file_path):
        raise FileNotFoundError(f"File not found: {abs_file_path}")
    return abs_file_path