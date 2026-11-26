"""Kiểm tra schema tài khoản bằng SQLite thực trong bộ nhớ."""

import hashlib
from pathlib import Path
import sqlite3
import unittest


SCHEMA = (Path(__file__).resolve().parents[1] / "db" / "schema.sql").read_text(
    encoding="utf-8"
)


class AccountsSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        salt = bytes.fromhex("c8aaec8115618877503a90243a641e44")
        digest = hashlib.pbkdf2_hmac("sha256", b"test-password", salt, 600_000)
        cls.password_hash = f"pbkdf2_sha256$600000${salt.hex()}${digest.hex()}"

    def setUp(self):
        self.connection = sqlite3.connect(":memory:")
        self.addCleanup(self.connection.close)
        self.connection.executescript(SCHEMA)
        self.connection.execute(
            "INSERT INTO accounts (username, password_hash) VALUES (?, ?)",
            ("pilot", self.password_hash),
        )
        self.connection.commit()

    def test_duplicate_username_is_rejected(self):
        with self.assertRaises(sqlite3.IntegrityError) as error:
            self.connection.execute(
                "INSERT INTO accounts (username, password_hash) VALUES (?, ?)",
                ("pilot", self.password_hash),
            )
        self.assertEqual(error.exception.sqlite_errorname, "SQLITE_CONSTRAINT_UNIQUE")
        self.assertEqual(
            self.connection.execute(
                "SELECT username, password_hash FROM accounts"
            ).fetchall(),
            [("pilot", self.password_hash)],
        )

    def test_different_username_is_accepted(self):
        self.connection.execute(
            "INSERT INTO accounts (username, password_hash) VALUES (?, ?)",
            ("surveyor", self.password_hash),
        )
        self.assertEqual(
            self.connection.execute("SELECT COUNT(*) FROM accounts").fetchone()[0], 2
        )

    def test_schema_can_be_applied_again_without_losing_accounts(self):
        self.connection.executescript(SCHEMA)
        self.assertEqual(
            self.connection.execute(
                "SELECT password_hash FROM accounts WHERE username = ?", ("pilot",)
            ).fetchone()[0],
            self.password_hash,
        )


if __name__ == "__main__":
    unittest.main()
