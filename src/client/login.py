"""Cửa sổ nhập thông tin đăng nhập cho Flying Survey."""

import tkinter as tk
from tkinter import ttk


class LoginWindow(ttk.Frame):
    def __init__(self, master):
        super().__init__(master, padding=24)
        self.grid(sticky="nsew")
        self.columnconfigure(0, weight=1)

        self.username = tk.StringVar(master=master)
        self.password = tk.StringVar(master=master)
        self.username_error = tk.StringVar(master=master)
        self.password_error = tk.StringVar(master=master)
        self.status = tk.StringVar(master=master)

        ttk.Label(self, text="Tên đăng nhập").grid(sticky="w")
        self.username_entry = ttk.Entry(self, textvariable=self.username)
        self.username_entry.grid(sticky="ew", pady=(4, 0))
        ttk.Label(self, textvariable=self.username_error, foreground="#b42318").grid(
            sticky="w", pady=(2, 10)
        )

        ttk.Label(self, text="Mật khẩu").grid(sticky="w")
        self.password_entry = ttk.Entry(self, textvariable=self.password, show="*")
        self.password_entry.grid(sticky="ew", pady=(4, 0))
        ttk.Label(self, textvariable=self.password_error, foreground="#b42318").grid(
            sticky="w", pady=(2, 10)
        )

        self.submit_button = ttk.Button(
            self, text="Đăng nhập", command=self.validate_fields
        )
        self.submit_button.grid(sticky="ew", pady=(0, 12))
        ttk.Label(self, textvariable=self.status, wraplength=320).grid(sticky="w")

        master.bind("<Return>", self.validate_fields)
        self.username_entry.focus_set()

    def validate_fields(self, event=None):
        username_missing = not self.username.get().strip()
        password_missing = not self.password.get().strip()
        self.username_error.set("Vui lòng nhập tên đăng nhập." if username_missing else "")
        self.password_error.set("Vui lòng nhập mật khẩu." if password_missing else "")
        self.status.set("")

        if username_missing:
            self.username_entry.focus_set()
            return False
        if password_missing:
            self.password_entry.focus_set()
            return False

        self.status.set("Đã nhập đủ thông tin. Chức năng xác thực với server đang chờ triển khai.")
        return True


def main():
    root = tk.Tk()
    root.title("Flying Survey - Đăng nhập")
    root.minsize(380, 320)
    root.columnconfigure(0, weight=1)
    root.rowconfigure(0, weight=1)
    LoginWindow(root)
    root.mainloop()


if __name__ == "__main__":
    main()
