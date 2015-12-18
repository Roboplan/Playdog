var homebases = [];
var bases = [];
var others = [];
var ready = false;
var loggedIn = false;
var activeDevice = null;
var timeout = 10;

var TIMEOUT_DELAY = 500;

function deviceModal(deviceName, deviceID) {
    $("#deviceModalTitle").html("Device " + deviceName + " (" + deviceID + ")");
    $("#deviceModal").modal()
    
    spark.getDevice(deviceID, function(err, device) {
        if (err)
            console.log("Failed to find device", err);
        else
            activeDevice = device;
        });
}

function updateTimeout()
{
    timeout = parseInt($("#inputTimeout").val());
    console.log("Timeout updated", timeout);
}

if (!Date.now) {
    Date.now = function() { return new Date().getTime(); }
}

var testPIRBtn = function()
{
    checkPIR(activeDevice, function (err, data) {
        console.log("TestPIR: err:", err, " data:", data);
        
        if (err)
        {
            $.notify(err, "error");
        }
        else
        {
            if (data.return_value == "1")
            {
                $.notify("Test executing", "info");
            }
            else
            {
                $.notify("Test failed:" + data.return_value, "error");
            }
        }
        
    });
}

var checkPIR = function(device, callback) {
    device.callFunction('AtomAction','Check PIR,10', callback);
};

var dispense = function(device, callback) {
    device.callFunction('AtomAction','Dispense', callback);
};

var checkDeviceHomeBase = function(device, devices, callback) {
    device.callFunction('Tester','Is HomeBase', function (err, data) {
        if (err)
        {
            console.log(err, "treating as other");
            others.push(device);
            device.type = "other";
        }
        else
        {
            var return_value = data.return_value;
            
            
            if (return_value == 1)
            {
                console.log("Found homebase");
                homebases.push(device);
                device.type = "homebase";
            }
            else if (return_value == 0)
            {
                console.log("Found base");
                bases.push(device);
                device.type = "base";
            }
            else
            {
                console.log("Found other");
                others.push(device);
                device.type = "other";
            }
        }
        
        checkDevices(devices, callback);
    });
};

var checkDevices = function(devices, callback) {
    //console.log("Devices:", devices, "hb:", homebases, "bases:", bases, "others:", others);
    if (devices.length > 0)
    {
        var device = devices.pop()
        //console.log("Checking:", device);
        if (device.connected)
        {
            //console.log("connected");
            checkDeviceHomeBase(device, devices, callback);
        }
        else
        {
            checkDevices(devices, callback);
        }
    }
    else
    {
        ready = true;
        callback();
    }
}

var getDevices = function(callback) {
    homebases = [];
    bases     = [];
    others    = [];
    
    var devicesPr = spark.listDevices();
    
    devicesPr.then(
        function(devices){
            console.log('Devices: ', devices);
            checkDevices(devices, callback);
        }, 
    
        function(err) {
            console.log('List devices call failed: ', err);
        });
};

var runEventCallback = null;

var runEventHandler = function(data)
{
    if (runEventCallback)
    {
        runEventCallback(data);
    }
    else
    {
        console.log("Ignoring run event", data);
    }
}

var stationRound = function(device, mode, timeout, callback) {
    /*
    mode    - Default/Reward/Cue
    timeout - in seconds
    callback- function(err, data)
    */
    
    console.log("round", device.id, mode);
    
    var params = mode + "," + timeout.toString();
    device.callFunction('Play',params, function (err, data) {
        if (err)
            callback(err, data);
        else
        {
            var timeoutProtect = setTimeout(function() 
            {
                timeoutProtect = null;
                console.log('Timeout');
                callback(new Error("Operation Timedout"), null);
            }, 1000*timeout + TIMEOUT_DELAY);

            runEventCallback = function(data) 
            {
                runEventCallback = null;
                if (timeoutProtect)
                {
                    clearTimeout(timeoutProtect);
                    var res = data.data.split(":");
                    
                    if (res[0] == "1")
                    {
                        console.log('Success');
                        callback(null, data);
                    }
                    else
                    {
                        console.log('Failed');
                        callback(new Error("Failed"), data);
                    }

                }
            };
        }
    });
}

if (!String.prototype.format) {
    String.prototype.format = function() {
        var str = this.toString();
        if (!arguments.length)
            return str;
        var args = typeof arguments[0],
            args = (("string" == args || "number" == args) ? arguments : arguments[0]);
        for (arg in args)
            str = str.replace(RegExp("\\{" + arg + "\\}", "gi"), args[arg]);
        return str;
    }
}

var refreshDevices = function()
{
    if (! loggedIn)
    {
        $.notify("Login first", "warn");
        return;
    }
    
    var devButton = function(device) {
        var res='<button type="button" class="btn btn-{deviceColor}" onclick=\'deviceModal("{deviceName}","{deviceId}")\'>{deviceName} {deviceType}</button>';
        
        return res.format({deviceColor: "primary", deviceName: device.name, deviceId: device.id, deviceType: device.type});
    }
    
    $.notify("Polling devices...", "info")
    getDevices(function() {
        $("#devices").html(homebases.map(devButton) + bases.map(devButton) + others.map(devButton));
        $.notify("Devices updated", "info");
    });
}

var doLogin = function()
{
    //e.preventDefault();
    var user = $('#spark-email').val();
    var pass = $('#spark-password').val();

    var loginPromise = window.spark.login({ username: user, password: pass });

    
    var hideLoginError = function() {
        $("#loginModalError").hide();
    }
    var setLoginError = function(text) {
        $("#loginModalError").html(text)
        $("#loginModalError").show();
    }
    
    loginPromise.then(
      function(data) {
        //callback(data);
        //$('#spark-email').val('');
        //$('#spark-password').val('');
        $.notify("Logged in", "success");
        //displayErrorMessage('');
        //$('#spark-login-form-error').hide();
        hideLoginError();
        $('#loginModal').modal('hide');
        
        loggedIn = true;
    
        spark.getEventStream('Run', 'mine', runEventHandler);
    
        refreshDevices();
        
      },
      function(error) {
        if (error.message === 'invalid_grant') {
          setLoginError('Invalid username or password.');
        } else if (error.cors === 'rejected') {
          setLoginError('Login Request rejected.');
        } else {
          setLoginError(error.message);
          console.log(error);
        }
      }
    );
}
/*sparkLogin(function(data) {
    $.notify("Logged in", "success");
    console.log(data);
    loggedIn = true;
    
    spark.getEventStream('Run', 'mine', runEventHandler);
    
    refreshDevices();
});*/


var dispenseBtn = function() {
    if (!ready)
    {
        $.notify("Login first", "warn");
        return;
    }
    
    if (homebases.length<1)
    {
        $.notify("Missing homebase", "error");
        return;
    }
    
    $.notify("Dispensing...", "info");
    dispense(homebases[0], function(err, data) {
        if (err)
            $.notify(err, "error");
        else
            $.notify("Dispensed", "success");
    })
};

var teachBtn = function() {
    if (!ready)
    {
        $.notify("Login first", "warn");
        return;
    }

    if (homebases.length<1)
    {
        $.notify("Missing homebases","error");
        return;
    }
    
    $.notify("Starting teach...", "info");
    stationRound(homebases[0], "Cue", timeout, function(err, data) {
        if (err)
            $.notify(err);
        else
            $.notify("Teach Success", "success");
    })
};

var singleBtn = function() {
    gameSequence(1);
};

var doubleBtn = function() {
    gameSequence(2);
};

var tripleBtn = function() {
    gameSequence(3);
};

var randomChoice = function(arr) {
  return arr[Math.floor(Math.random()*arr.length)];  
};

var doSeq = function(seq) {
    if (seq.length == 1)
    {
        var homebase = seq.pop()
        
        stationRound(homebase, "Reward", timeout, function(err, data) {
            if (err)
            {
                $.notify(err, "error");
                return;
            }
            else
            {
                var res = data.data.split(":");
                
                if (res[0] == "1")
                {
                    $.notify("Game Success", "success");
                    return;
                }
                else
                {
                    $.notify("Game Failed", "warn");
                    return;
                }
            }
        });
    }
    else
    {
        var base = seq.pop()
        
        stationRound(base, "Default", timeout, function(err, data) {
            if (err)
            {
                $.notify(err, "error");
                return
            }
            else
            {
                var res = data.data.split(":");
                
                if (res[0] == "1")
                {
                    $.notify("Base reached, continue...", "success");
                    doSeq(seq);
                }
                else
                {
                    $.notify("Base missed. Failed", "warn");
                    return
                }
            }
        });
    }
}

var gameSequence = function(len) {
    if (!ready)
    {
        $.notify("Login first", "warn");
        return;
    }

    if (homebases.length<1)
    {
        $.notify("Missing homebases", "error");
        return;
    }

    if (len>1 && bases.length<1)
    {
        $.notify("Missing bases", "error");
        return;
    }
    
    $.notify(len.toString() + "-Game...", "info");
    
    var seq = [homebases[0]];
    
    if (len>1)
    {
        var lastBase = seq[0];
        var pool = homebases.concat(bases);
        
        for (var i = 0; i < len-1; i++) { 
            // add an element which is different from the last one
            var lastIndex = pool.indexOf(lastBase);
            var curPool = pool.slice();
            curPool.splice(lastIndex, 1);
            var base = randomChoice(curPool);
            seq.push(base);
            
            lastBase = base;
        }

        console.log(seq);
    }
    
    doSeq(seq);
};

var flash = function(device, files) {
    //var file = loadFile('http://www.google.com');
    getFile("text.txt", function(content) {
        console.log("Flashing ", content);
        
        var baseUrl = 'https://api.particle.io';
    
        var r = this.request({
            uri: baseUrl + '/v1/devices/' + device.id + '?access_token=' + accessToken,
            method: 'PUT',
            json: true
        }, callback);
        
        
    });
    //console.log(file);
    return;
    
    
        
    var form = r.form(),
    paramName = 'file';
        
    for (var i in files) {
        form.append(paramName, fs.createReadStream(path.join(process.cwd(), files[i])), {
        filename: path.basename(files[i])
        });
        paramName = 'file' + i;
        }

}
function getFile(url, callback) {
    var xmlhttp;
    
    if (window.XMLHttpRequest) {
        xmlhttp = new XMLHttpRequest();
    } else {
        // code for older browsers
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    
    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4)
        {
            if (xmlhttp.status == 200) 
            {
                callback(xmlhttp.responseText);
            }
            else
            {
                console.log("getFile failed", xmlhttp.readyState, xmlhttp.status);
            }
        }
    };
    
    xmlhttp.open("GET", url, true);
    xmlhttp.send();
    
}