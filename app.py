import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os


# import psutil
# for p in psutil.disk_partitions():
#     print(p.device, p.mountpoint)

root = tk.Tk()
root.title("Secure File Shredder")
root.geometry("700x400")

frm = ttk.Frame(root, padding=15)
frm.pack(fill=tk.BOTH, expand=True)

selected_files = []

def update_button_state():
    if selected_files:
        remove_btn.config(state="normal")
        remove_all_btn.config(state="normal")
        fast_shred_btn.config(state="normal")
        secure_shred_btn.config(state="normal")
    else:
        remove_btn.config(state="disabled")
        remove_all_btn.config(state="disabled")
        fast_shred_btn.config(state="disabled")
        secure_shred_btn.config(state="disabled")

def refresh_tree():
    for item in tree.get_children():
        tree.delete(item)
    for idx, path in enumerate(selected_files, start=1):
        tree.insert("", "end", values=(idx, os.path.basename(path), path))
    update_button_state()

def select_files():
    paths = filedialog.askopenfilenames()
    if paths:
        for p in paths:
            if p not in selected_files:
                selected_files.append(p)
        refresh_tree()

def remove_file():
    selected = tree.selection()
    if selected:
        for item in selected:
            index = int(tree.item(item)["values"][0]) - 1
            selected_files.pop(index)
        refresh_tree()

def remove_all_files():
    selected_files.clear()
    refresh_tree()

def shred(mode: int):
    if not selected_files:
        messagebox.showwarning("No files", "Please select files first")
        return
    messagebox.showinfo("Shred", f"Shredding {len(selected_files)} files with mode {mode}")
    remove_all_files()


# ---- Layout ----

btn_frame = ttk.Frame(frm)
btn_frame.grid(row=0, column=0, columnspan=3, pady=5, sticky="ew")

ttk.Button(btn_frame, text="󰐕 Add Files", command=select_files).pack(side="left", padx=5)
remove_btn = ttk.Button(btn_frame, text="󰧧 Remove Selected", command=remove_file)
remove_btn.pack(side="left", padx=5)
remove_all_btn = ttk.Button(btn_frame, text="󰧧 Clear All", command=remove_all_files)
remove_all_btn.pack(side="left", padx=5)

columns = ("#", "File", "Path")
tree = ttk.Treeview(frm, columns=columns, show="headings", selectmode="extended")
tree.heading("#1", text="#")
tree.heading("#2", text="Filename", anchor="w")
tree.heading("#3", text="Path", anchor="w")
tree.column("#1", width=30, anchor="center")
tree.column("#2", width=250)
tree.column("#3", width=380)

scrollbar = ttk.Scrollbar(frm, orient="vertical", command=tree.yview)
tree.configure(yscrollcommand=scrollbar.set)
tree.grid(row=1, column=0, columnspan=2, pady=10, sticky="nsew")
scrollbar.grid(row=1, column=2, sticky="ns")

shred_frame = ttk.Frame(frm)
shred_frame.grid(row=2, column=0, columnspan=3, pady=10)

fast_shred_btn = ttk.Button(shred_frame, text="Fast Shred", command=lambda: shred(1))
fast_shred_btn.pack(side="left", padx=20)
secure_shred_btn = ttk.Button(shred_frame, text="Secure Shred", command=lambda: shred(2))
secure_shred_btn.pack(side="left", padx=20)

status_var = tk.StringVar(value="No files selected")
status_label = ttk.Label(frm, textvariable=status_var, anchor="w")
status_label.grid(row=3, column=0, columnspan=3, sticky="ew")

frm.rowconfigure(1, weight=1)
frm.columnconfigure(0, weight=1)

update_button_state()

root.mainloop()
