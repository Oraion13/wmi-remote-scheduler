const username = document.getElementById("username");
const password = document.getElementById("password");
const computer_name = document.getElementById("computer_name");

const login_form = document.getElementById("login_form");

// initially, check if a machine is connected
window.addEventListener("DOMContentLoaded", function(){
    if(this.window.localStorage.getItem("wmi_user")){
        this.document.getElementById("already_connected").classList.remove("connected");
    }else{
        this.document.getElementById("already_connected").classList.add("connected");
    }
})
// login form listener
login_form.addEventListener("submit", login);
// logout button
document.getElementById("logout").addEventListener("click", function(){
    if(window.confirm("Are You Sure to Disconnect?")){
        window.localStorage.removeItem("wmi_user");
        window.location.replace("./index.html");
    }
});
// go to post tasks page
document.getElementById("goto_machine").addEventListener("click", function(){
    window.location.replace("./remote_tasks.html");
});

// POST login data
function login(e) {
    e.preventDefault();

    const Login = {
        username: username.value,
        password: password.value,
        computer_name: computer_name.value
    };

    console.log("Login", Login);
    const xhr = new XMLHttpRequest();
    xhr.open("POST", "./api/remote_tasks.cgi", true);

  xhr.onreadystatechange = function () {
    if (xhr.readyState == XMLHttpRequest.DONE) {
      const got = JSON.parse(xhr.responseText);

      if (got.error) {
        window.alert(got.error);
      } else {
        window.localStorage.setItem("wmi_user", JSON.stringify(Login));
        window.location.replace("./remote_tasks.html");
      }
    }
  };
  xhr.send(JSON.stringify(Login));
}