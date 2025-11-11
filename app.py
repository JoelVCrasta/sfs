import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os
from ctypes_binds import shred_files_cpp

root = tk.Tk()
root.title("Secure File Shredder")
root.geometry("700x400")

frm = ttk.Frame(root, padding=15)
frm.pack(fill=tk.BOTH, expand=True)

selected_files = []
status_var = tk.StringVar(value="No files selected")

def update_status():
    count = len(selected_files)
    if count == 0:
        status_var.set("No files selected")
    elif count == 1:
        status_var.set("1 file selected")
    else:
        status_var.set(f"{count} files selected")

def update_button_state():
    state = "normal" if selected_files else "disabled"
    remove_btn.config(state=state)
    remove_all_btn.config(state=state)
    fast_shred_btn.config(state=state)
    secure_shred_btn.config(state=state)
    update_status()

def refresh_tree():
    for item in tree.get_children():
        tree.delete(item)
    for path in selected_files:
        try:
            size = os.path.getsize(path)
        except FileNotFoundError:
            size = "?"
        tree.insert("", "end", values=(os.path.basename(path), path, size))
    update_button_state()

def select_files():
    paths = filedialog.askopenfilenames(title="Select files")
    if paths:
        for p in paths:
            if p not in selected_files:
                selected_files.append(p)
        refresh_tree()

def remove_file():
    for item in tree.selection():
        path = tree.item(item)["values"][1]
        if path in selected_files:
            selected_files.remove(path)
        tree.delete(item)
    update_button_state()

def remove_all_files():
    selected_files.clear()
    refresh_tree()

def shred(mode: int):
    if not selected_files:
        messagebox.showwarning("No files", "Please select files first")
        return
    err_count = shred_files_cpp(selected_files, mode)
    if err_count == -1:
        messagebox.showerror("Error", "Invalid input to shred library")
    else:
        messagebox.showinfo("Shred Complete", f"Shredded {len(selected_files)} files.\nErrors: {err_count}")
    remove_all_files()
    status_var.set(f"Last operation: {err_count} errors")

btn_frame = ttk.Frame(frm)
btn_frame.grid(row=0, column=0, columnspan=3, pady=5, sticky="ew")

ttk.Button(btn_frame, text="󰐕 Add Files", command=select_files).pack(side="left", padx=5)
remove_btn = ttk.Button(btn_frame, text="󰧧 Remove Selected", command=remove_file)
remove_btn.pack(side="left", padx=5)
remove_all_btn = ttk.Button(btn_frame, text="󰧧 Clear All", command=remove_all_files)
remove_all_btn.pack(side="left", padx=5)

columns = ("File", "Path", "Size")
tree = ttk.Treeview(frm, columns=columns, show="headings", selectmode="extended")
tree.heading("#1", text="Filename", anchor="w")
tree.heading("#2", text="Path", anchor="w")
tree.heading("#3", text="Size", anchor="w")
tree.column("#1", width=250)
tree.column("#2", width=380)
tree.column("#3", width=80)

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

status_label = ttk.Label(frm, textvariable=status_var, anchor="w")
status_label.grid(row=3, column=0, columnspan=3, sticky="ew")

frm.rowconfigure(1, weight=1)
frm.columnconfigure(0, weight=1)

update_button_state()
root.mainloop()
