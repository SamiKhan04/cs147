CREATE DATABASE cs147;

USE cs147;

CREATE TABLE users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password VARCHAR(255) NOT NULL,
    rfid VARCHAR(50) NULL
);

CREATE TABLE user_stats (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    login_attempts INT DEFAULT 0,
    last_login_time DATETIME DEFAULT NULL,
    open_lock_attempts INT DEFAULT 0,
    last_lock_open DATETIME DEFAULT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

INSERT INTO users (username, password)
VALUES
    ('test1', '12345'),
    ('test2', '67890');

INSERT INTO user_stats (user_id, login_attempts, last_login_time)
SELECT id, 0, NULL FROM users;

UPDATE users set rfid = "C1 CD 24 3";