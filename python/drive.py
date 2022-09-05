import glob
import sys
from pydrive.auth import GoogleAuth
from pydrive.drive import GoogleDrive


folder_path = sys.argv[1]
folder_name = folder_path.replace("\\", "/").split("/")[-1]
destination = sys.argv[2].replace("#"," ")


def get_folder_id(drive, path, sharedWithMe=False):
    path_parts = path.split("/")
    parent = "root"
    swm = ""
    inParent = ""
    if sharedWithMe:
        swm="and sharedWithMe"
    else:
        inParent="'root' in parents and"

    for part in path_parts:
        query = "%s title='%s' %s and mimeType='application/vnd.google-apps.folder' and trashed=false" % (inParent, part, swm)
        folder_list = drive.ListFile({"q": query}).GetList()
        swm = ""
        if len(folder_list) == 0:
            return None
        parent = folder_list[0]["id"]
        inParent = "'%s' in parents and " % (parent, )
    return parent


gauth = GoogleAuth()
gauth.LocalWebserverAuth()
drive = GoogleDrive(gauth)
folder_id = get_folder_id(drive, destination, True)

#Create folder
folder_metadata = {'title': folder_name, 'parents': [{"id": folder_id}], 'mimeType': 'application/vnd.google-apps.folder'}
folder = drive.CreateFile(folder_metadata)
folder.Upload()
new_folder_id = get_folder_id(drive, destination + "/" + folder_name, True)

for file_name in glob.glob(folder_path + "/*"):
    new_file_metadata = {'title': file_name.replace("\\", "/").split("/")[-1], 'parents': [{"id": new_folder_id}]}
    new_file = drive.CreateFile(new_file_metadata)
    # Read file and set it as a content of this instance.
    new_file.SetContentFile(file_name)
    new_file.Upload() # Upload the file.

