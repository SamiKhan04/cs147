from flask import Flask, render_template, request, redirect, url_for, flash, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager, UserMixin, login_user, login_required, logout_user, current_user
import os
from datetime import datetime

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+mysqlconnector://texerauser:12345@localhost/cs147'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY')

db = SQLAlchemy(app)

# Initialize Flask-Login
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'

# db = {"sami": ("12345", "C1 CD 24 3")}

class User(UserMixin, db.Model):
    __tablename__ = 'users'
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(50), nullable=False, unique=True)
    password = db.Column(db.String(255), nullable=False)
    rfid = db.Column(db.String(50), nullable=True)

    def __repr__(self):
        return f'<User: {self.username}, RFID: {self.rfid}>'

class UserStats(db.Model):
    __tablename__ = 'user_stats'
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'), nullable=False)
    login_attempts = db.Column(db.Integer, default=0)
    last_login_time = db.Column(db.DateTime, default=None)
    open_lock_attempts = db.Column(db.Integer, default=0)
    last_lock_open = db.Column(db.DateTime, default=None)

    def __repr__(self):
        return (f"<UserStats user_id={self.user_id}, login_attempts={self.login_attempts}, "
                f"last_login_time={self.last_login_time}, open_lock_attempts={self.open_lock_attempts}, "
                f"last_lock_open={self.last_lock_open}>")


with app.app_context():
    db.create_all()
    

@app.route('/user')
def get_users():
    users = User.query.all()
    
    print({'users': [{'id': user.id, 'username': user.username, 'password': user.password} for user in users]})
    
    return {'users': [{'id': user.id, 'username': user.username, 'password': user.password} for user in users]}

@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))




# Routes
@app.route('/')
def home():
    return f"Welcome, {current_user.username}!" if current_user.is_authenticated else "Welcome, Guest!"

def record_login_attempt(user):
    # Increment login attempts
    stats = UserStats.query.filter_by(user_id=user.id).first()
    if stats:
        stats.login_attempts += 1
        stats.last_login_time = datetime.utcnow()
        db.session.commit()

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        user = User.query.filter_by(username=username).first()
        
        if user and user.password == password:  # Replace this with a hashed password comparison
            login_user(user)
            record_login_attempt(user)
            flash('Login successful!', 'success')
            return redirect(url_for('protected'))
        else:
            flash('Invalid username or password.', 'danger')

    return render_template('login.html')


def record_lock_attempt(user):
    stats = UserStats.query.filter_by(user_id=user.id).first()
    if not stats:
        stats = UserStats(user_id=user.id, open_lock_attempts=1, last_lock_open=datetime.utcnow())
        db.session.add(stats)
    else:
        stats.open_lock_attempts += 1
        stats.last_lock_open = datetime.utcnow()
    db.session.commit()
    
@app.route('/lock_attempt', methods=['GET'])
def record_lock_attempt():
    rfid = request.args.get('rfid')  # Get the RFID from query parameters
    if not rfid:
        return jsonify({'error': 'RFID is required'}), 400

    # Find the user associated with this RFID
    user = User.query.filter_by(rfid=rfid).first()
    if not user:
        return jsonify({'error': 'User with this RFID not found'}), 404

    # Record the lock attempt in user_stats
    stats = UserStats.query.filter_by(user_id=user.id).first()
    if not stats:
        stats = UserStats(user_id=user.id, open_lock_attempts=1, last_lock_open=datetime.utcnow())
        db.session.add(stats)
    else:
        stats.open_lock_attempts += 1
        stats.last_lock_open = datetime.utcnow()
    db.session.commit()

    return jsonify({
        'message': 'Lock attempt recorded',
        'user': {
            'id': user.id,
            'username': user.username
        },
        'stats': {
            'open_lock_attempts': stats.open_lock_attempts,
            'last_lock_open': stats.last_lock_open
        }
    }), 200


@app.route('/protected')
@login_required
def protected():
    return f"Hello {current_user.username}, this is a protected route!"

@app.route('/logout')
@login_required
def logout():
    logout_user()
    flash('You have been logged out.', 'info')
    return redirect(url_for('login'))


if __name__ == '__main__':
    app.run(debug=True)