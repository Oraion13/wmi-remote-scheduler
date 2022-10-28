const remote_tasks_form = document.getElementById("remote_tasks_form");

const title = document.getElementById("title");
const description = document.getElementById("description");
const start_time = document.getElementById("start_time");
const run_repeatedly = document.getElementById("run_repeatedly");
const if_repeated = document.getElementById("if_repeated");
const days_of_week = document.getElementById("days_of_week");
const days_of_month = document.getElementById("days_of_month");

// initially, check if a machine is connected
window.addEventListener("DOMContentLoaded", function() {
    if(!get_user_token()["username"]){
        this.window.alert("No Remote Machine connected");
        this.window.location.replace("./index.html");
    }
})
// remote tasks form
remote_tasks_form.addEventListener("submit", remote_tasks);
// home button
document.getElementById("home").addEventListener("click", function(){
    if(window.confirm("Leave Home Page?")){
        window.location.replace("./index.html");
    }
});
// logout button
document.getElementById("logout").addEventListener("click", function(){
    if(window.confirm("Are You Sure to Disconnect?")){
        window.localStorage.removeItem("wmi_user");
        window.location.replace("./index.html");
    }
})

// get user details from local storage
const get_user_token = () => {
    return window.localStorage.getItem("wmi_user")
        ? JSON.parse(window.localStorage.getItem("wmi_user"))
        : {};
};

// ***************************** Hide/Show Repeadted form *********************** //
run_repeatedly.addEventListener("change", function (e) {
    e.preventDefault();

    if (run_repeatedly.value == 0) {
        if_repeated.classList.add("its_repeated");
    } else {
        if_repeated.classList.remove("its_repeated");
    }
});

// POST task
function remote_tasks(e) {
    e.preventDefault();

    // check if user token exists
    const user_token = get_user_token();
    if (!user_token["username"]) {
        window.alert("No Remote PC Connected!")
        return;
    }

    // modify start time
    var stime = start_time.value.replace(/(-|T|:)/g, "") + "00.000000+330";

    // prepare the req body
    const RemoteTasks = [
        {
            username: user_token["username"],
            password: user_token["password"],
            computer_name: user_token["computer_name"]
        },
        {
            title: title.value,
            description: description.value,
            start_time: stime,
            run_repeat: run_repeatedly.value == 1 ? true : false,
            days_of_week: parseInt(days_of_week.value),
            days_of_month: parseInt(days_of_month.value)
        }
    ];

    console.log("RemoteTasks", RemoteTasks);
    
    const xhr = new XMLHttpRequest();
    xhr.open("POST", "./api/remote_tasks.cgi", true);

    xhr.onreadystatechange = function () {
        if (xhr.readyState == XMLHttpRequest.DONE) {
            const got = JSON.parse(xhr.responseText);

            if (got.error) {
                window.alert(got.error);
            } else {
                window.alert("Task Added Successfully!");

            }
        }
    };
    xhr.send(JSON.stringify(RemoteTasks));
}
