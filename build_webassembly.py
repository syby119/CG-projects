import argparse
import multiprocessing
import os
import re
import shutil
import subprocess


# get the absolute directory of this script
def get_absolute_path(name):
    script_abs_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(script_abs_dir, name)


# execute a shell cmd
def exec(cmd):
    print(cmd)
    popen = subprocess.Popen(
        args=cmd,
        stderr=subprocess.STDOUT,
        stdout=subprocess.PIPE,
        shell=True,
    )

    while popen.poll() is None:
        line = popen.stdout.readline().decode("utf8")
        if line:
            print(line, end='')

    print("process exit with {}\n".format(popen.returncode))

    return popen.returncode


# parse command line args
def parse_cmd_line():
    parser = argparse.ArgumentParser(
        description=
            "This script is to build projects running in web brower\n\n"
            "Prerequisites:\n"
            "+ CMake >= 3.20\n"
            "+ Emscripten: https://emscripten.org\n"
            "+ Python >= 3.5\n",
        formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument(
        "-D", "--directory",
        default="build",
        help="directory to contain binary")

    parser.add_argument(
        "-P", "--project",
        default="", help="project to be built")

    args = parser.parse_args()

    build_dir = args.directory
    target_proj = args.project

    return build_dir, target_proj


# build the project with CMake and Emscripten
def build_web(build_dir, target_proj):
    cmd = "emcmake cmake -B{} {} -DUSE_GLES=1".format(build_dir, get_absolute_path("."))
    result = exec(cmd)
    if result != 0:
        err_message = "{} failure with {}".format(cmd, result)
        raise Exception(err_message)

    cmd = "cmake --build {} --parallel {}".format(build_dir, multiprocessing.cpu_count())
    if target_proj:
        cmd += " --target {}".format(target_proj)

    result = exec(cmd)
    if result != 0:
        err_message = "{} failure with {}".format(cmd, result)
        raise Exception(err_message)


# get the name of the projects
def get_project_names(projects_dir):
    project_names = []
    pattern = re.compile(r'project\s*\(\s*[a-z0-9_\-]+\s*\)', re.I)
    for root, dirs, files in os.walk(projects_dir):
        for name in files:
            if name == "CMakeLists.txt":
                filepath = os.path.join(root, name)
                with open(filepath) as fp:
                    while True:
                        line = fp.readline()
                        if not line:
                            break
                        result = pattern.search(line)
                        if result:
                            project_name = result[0][7: -1].strip()[1:].strip()
                            project_names.append(project_name)
                            break
    return project_names


# get project configuration
def get_project_config(projects_dir, project_name):
    config_main = os.path.join(projects_dir, project_name, "main.cpp")
    config = { "script": project_name + ".js" }
    with open(config_main, "r") as fp:
        while True:
            line = fp.readline()
            if not line:
                break

            if not config.get("windowTitle"):
                index = line.find(".windowTitle")
                if index >= 0:
                    config["title"] = line[line.find("=") + 1:].split('"')[1]
                    continue

            if not config.get("canvasWidth"):
                index = line.find(".windowWidth")
                if index >= 0:
                    config["canvasWidth"] = int(line[line.find("=") + 1:].split(";")[0])
                    continue

            if not config.get("canvasHeight"):
                index = line.find(".windowHeight")
                if index >= 0:
                    config["canvasHeight"] = int(line[line.find("=") + 1:].split(";")[0])
                    continue

    return config


# generate html files for all projects
def generate_html(config, html_template_file):
    html = ""
    pattern = re.compile(r'{{\s*[a-z0-9_]+\s*}}', re.I)

    with open(html_template_file) as html_template:
        for line in html_template.readlines():
            results = pattern.findall(line)
            if results:
                for result in results:
                    keyword = result.strip("{").strip("}").strip()
                    if keyword == "title":
                        line = line.replace(result, config["title"])
                    elif keyword == "canvasWidth":
                        line = line.replace(result, str(config["canvasWidth"]) + "px")
                    elif keyword == "canvasHeight":
                        line = line.replace(result, str(config["canvasHeight"]) + "px")
                    elif keyword == "script":
                        line = line.replace(result, '"' + config["script"] + '"')

            html += line

    return html


# save the project dependent html
def save_html(html, build_dir, project_name):
    filepath = os.path.join(build_dir, "bin/browser", project_name + ".html")
    with open(filepath, "w+") as fp:
        fp.write(html)


# copy the run_webassembly.py script to the build directory
def copy_runbat(script_file, build_dir):
    filename = os.path.basename(script_file)
    dst = os.path.join(build_dir, "bin", "browser", filename)
    shutil.copy(script_file, dst)


# entry point
def main():
    build_dir, target_proj = parse_cmd_line()

    if not os.path.exists(build_dir):
        os.mkdir(build_dir)

    build_web(build_dir, target_proj)

    projects_dir = get_absolute_path("projects")

    project_names = []
    if target_proj:
        project_names.append(target_proj)
    else:
        project_names = get_project_names(projects_dir)

    html_template_file = get_absolute_path("projects/browser/index.handlebars")
    for project_name in project_names:
        config = get_project_config(projects_dir, project_name)
        html = generate_html(config, html_template_file)
        save_html(html, build_dir, project_name)

    script_file = get_absolute_path("projects/browser/run_webassembly.py")
    copy_runbat(script_file, build_dir)


if __name__ == "__main__":
    main()