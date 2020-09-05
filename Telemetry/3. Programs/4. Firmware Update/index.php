<?php
session_start();
?>

<html>
<head><title>Firmware Control Window</title>
<link rel="stylesheet" type="text/css" href="component.css" />
<script src="modernizr.custom.js"></script>

<style type="text/css">
div.outrect
{
 width : 1050;
 background : LightSeaGreen;
 font-size : 22px;
 font-family : Lucida Sans Unicode;
 box-shadow: 12px 12px 10px gray;
 border-radius: 15px;
 border:10px ridge green;
 margin: 10px;
}

div.inrect
{
 width : 1000px;
 padding : 1px;
 margin : 10px;
 background : Gray;
}

div.out
{
 padding : 10px;
 color:#223344;
 background: lightblue;
 margin : 1px;
 font-size : 15px;
 font-family : Verdana;
}

 div.title
  {
   width:998px;
   height : 25;
   padding-top : 4;
   margin-bottom : 4;
   margin-top : 1;
   color : Black ;
   border-radius : 5px;
   background: linear-gradient(gray, white, gray);
   font-size : 16;
   font-family : Lucida Sans Unicode;
   text-align : center;
   letter-spacing:1;
  }

  div.bgimg
  {
    background-image:url(img1.png);
    background-position:center;
    width:553px;
    height:77px;
	margin-bottom: 20px;
  }

  hr
  {
    color: black;
    background-color: black;
    height: 2px;
  }

  input:hover
  {
    opacity: 0.8;
    filter: alpha(opacity=80);
  }

  input
  {
    opacity: 1.0;
    filter: alpha(opacity=100);
  }

</style>
</head>

<script>
function selectAll(c)
{
     for (i = 1; i <= 35; i++)
	 {
      document.getElementById('chk' + i).checked = c;
     }
}
function AllC(c)
{
     for (i = 1; i <= 14; i++)
	 {
      document.getElementById('chk' + i).checked = c;
     }
}
function AllE(c)
{
     for (i = 15; i <= 19; i++)
	 {
      document.getElementById('chk' + i).checked = c;
     }
}
function AllS(c)
{
     for (i = 20; i <= 24; i++)
	 {
      document.getElementById('chk' + i).checked = c;
     }
}
function AllW(c)
{
     for (i = 25; i <= 30; i++)
	 {
      document.getElementById('chk' + i).checked = c;
     }
}

</script>

<body>
<br>
  <div class="md-modal md-effect-4" id="modal-4">
	<div class="md-content">
		<h3>REMOTE FIRMWARE UPDATE</h3>
		<p align="center"><font color="skyblue">About: Remote Firmware Update (v2.0). 27th May 2015 by Rahul D. Bhor</font></p>
		<div>
			 <font color="white" size="4">Procedure to update firmware of selected devices:</font>
			 <ul>
			 <li><font color="white" size="3">Step1: </font>Select System (GAB system is preselected).</li>
			 <li><font color="white" size="3">Step2: </font>Select Antennas.</li>
			 <li><font color="white" size="3">Step3: </font>Click on the 'Connect' button.</li>
			 <p>It will connect and display information of selected devices.</p>
			 <li><font color="white" size="3">Step4: </font>Click on the 'Status' button.</li>
			 <p>It will show status of last firmware update process.</p>
			 <li><font color="white" size="3">Step5: </font>Browse and upload firmware file (only .bin) to server.</li>
			 <p>(To do this you will need to authenticate yourself)</p>
			 <li><font color="white" size="3">Step6: </font>Click on the 'Install' button.</li>
			 </ul>
			 <button class="md-close">Close</button>
		</div>
	</div>
  </div>
</body>

<body bgcolor="silver">

<?php
$xmlDoc = new DOMDocument();
$xmlDoc->load("gablist.xml");
$x = $xmlDoc->documentElement;
$k=1;
foreach ($x->childNodes AS $item)
{
  $iplist = $item->nodeValue;
  if(!empty($iplist))
  {
  $iplist1[$k]=$iplist;
  $k++;
  }
}

if(isset($_POST['reset']))
{
session_unset();
session_destroy();
$_SESSION["system"]="   ";
for($i=0;$i<30;$i++)
  {
	$_SESSION["f1"][$i]="   ";
	$_SESSION["d1"][$i]="   ";
	$_SESSION["t1"][$i]="   ";
	$_SESSION["f2"][$i]="   ";
	$_SESSION["d2"][$i]="   ";
	$_SESSION["t2"][$i]="   ";
	$_SESSION["installed"]=0;
  }
}

$_SESSION['login1'] = false;
if(isset($_POST['upload']))
{
if (isset($_SERVER['PHP_AUTH_USER']) && isset($_SERVER['PHP_AUTH_PW']))
{
    $Username = $_SERVER['PHP_AUTH_USER'];
    $Password = $_SERVER['PHP_AUTH_PW'];
    if ($Username == 'rahul' && $Password == 'rcm4300')
	{
    $_SESSION['login1'] = true;
    }
}

if (!$_SESSION['login1'])
{
    header('WWW-Authenticate: Basic realm="fwup"');
    header('HTTP/1.0 401 Unauthorized');
    echo '<script> alert(\'Login failed!\'); </script>';
}
}

if(empty($_SESSION["installed"]))
$_SESSION["installed"]=0;
?>

<p>
<center>
<div class="outrect">
<div class="bgimg" style = "color:MintCream">
<pre><br><b style="margin-left:120px">Remote Firmware Update</b><font size="4" color="yellow"><b> v2.0</b></font><button class="md-trigger" data-modal="modal-4" style="margin-left:250px">Help</button></pre>

<div class="md-overlay"></div>
<script src="classie.js"></script>
<script src="modalEffects.js"></script>
</div>

<div class="inrect">
<div class="title"> <b>  Select Target Devices  </b>
<br><br>
<form id="reset" action="" method="post">
<input type="submit" name="reset" value="Reset" style="margin-left:910px" >
</form>
</div>

   <div class="out">
      <table style="width:90%">
        <tr>
           <form action="" method="post">
		   <td>
		   <pre><font size="5"><b>Select System</b></font></pre>
           <div style="overflow: auto; width: 160px; height: 146px; border: 2px solid green; background: rgb(213,206,166); padding-left: 5px">
			<br>
			<input type="radio" name="sys[]" value="GAB" checked>GAB <br>
            <input type="radio" name="sys[]" value="SENTINEL" >SENTINEL <br>
            <input type="radio" name="sys[]" value="FE" >FE <br>
            <input type="radio" name="sys[]" value="OF" >OF <br>
            <input type="radio" name="sys[]" value="FPS" >FPS <br><br>
           </div>
           </td>
           <td><pre>        </pre></td>
		   <td>
		   <pre><font size="5"><b>       Select Antenna</b></font></pre>
            <div style="overflow: auto; width: 320px; height: 260px; border: 2px solid green; background: rgb(213,206,166); padding-left: 2px; padding-right: 2px">
			<hr>
			<font color="blue"><input id="chk31" type="checkbox" onchange="selectAll(this.checked);">All</b></font>
			<font color="#079CB3"><input id="chk32" type="checkbox" onchange="AllC(this.checked);">All C</font>
			<font color="#A207B3"><input id="chk33" type="checkbox" onchange="AllE(this.checked);">All E</font>
			<font color="#38B307"><input id="chk34" type="checkbox" onchange="AllS(this.checked);">All S</font>
			<font color="#DF7906"><input id="chk35" type="checkbox" onchange="AllW(this.checked);">All W</font><br>
			<font color="#079CB3">
			<hr>
			<input id="chk1" type="checkbox" name="ant[]" value='<?php echo $iplist1[2]; ?>' >C00
            <input id="chk2" type="checkbox" name="ant[]" value='<?php echo $iplist1[4]; ?>' >C01
            <input id="chk3" type="checkbox" name="ant[]" value='<?php echo $iplist1[6]; ?>' >C02
            <input id="chk4" type="checkbox" name="ant[]" value='<?php echo $iplist1[8]; ?>' >C03
            <input id="chk5" type="checkbox" name="ant[]" value='<?php echo $iplist1[10]; ?>' >C04 <br>
            <input id="chk6" type="checkbox" name="ant[]" value='<?php echo $iplist1[12]; ?>' >C05
            <input id="chk7" type="checkbox" name="ant[]" value='<?php echo $iplist1[14]; ?>' >C06
            <input id="chk8" type="checkbox" name="ant[]" value='<?php echo $iplist1[16]; ?>' >C08
            <input id="chk9" type="checkbox" name="ant[]" value='<?php echo $iplist1[18]; ?>' >C09
            <input id="chk10" type="checkbox" name="ant[]" value='<?php echo $iplist1[20]; ?>' >C10 <br>
            <input id="chk11" type="checkbox" name="ant[]" value='<?php echo $iplist1[22]; ?>' >C11
            <input id="chk12" type="checkbox" name="ant[]" value='<?php echo $iplist1[24]; ?>' >C12
            <input id="chk13" type="checkbox" name="ant[]" value='<?php echo $iplist1[26]; ?>' >C13
            <input id="chk14" type="checkbox" name="ant[]" value='<?php echo $iplist1[28]; ?>' >C14 </font><br><hr>
			<font color="#A207B3">
            <input id="chk15" type="checkbox" name="ant[]" value='<?php echo $iplist1[30]; ?>' >E02
            <input id="chk16" type="checkbox" name="ant[]" value='<?php echo $iplist1[32]; ?>' >E03
            <input id="chk17" type="checkbox" name="ant[]" value='<?php echo $iplist1[34]; ?>' >E04
            <input id="chk18" type="checkbox" name="ant[]" value='<?php echo $iplist1[36]; ?>' >E05
            <input id="chk19" type="checkbox" name="ant[]" value='<?php echo $iplist1[38]; ?>' >E06 </font><br><hr>
			<font color="#38B307">
            <input id="chk20" type="checkbox" name="ant[]" value='<?php echo $iplist1[40]; ?>' >S01
            <input id="chk21" type="checkbox" name="ant[]" value='<?php echo $iplist1[42]; ?>' >S02
            <input id="chk22" type="checkbox" name="ant[]" value='<?php echo $iplist1[44]; ?>' >S03
            <input id="chk23" type="checkbox" name="ant[]" value='<?php echo $iplist1[46]; ?>' >S04
            <input id="chk24" type="checkbox" name="ant[]" value='<?php echo $iplist1[48]; ?>' >S06 </font><br><hr>
			<font color="#DF7906">
            <input id="chk25" type="checkbox" name="ant[]" value='<?php echo $iplist1[50]; ?>' >W01
            <input id="chk26" type="checkbox" name="ant[]" value='<?php echo $iplist1[52]; ?>' >W02
            <input id="chk27" type="checkbox" name="ant[]" value='<?php echo $iplist1[54]; ?>' >W03
            <input id="chk28" type="checkbox" name="ant[]" value='<?php echo $iplist1[56]; ?>' >W04 <br>
            <input id="chk29" type="checkbox" name="ant[]" value='<?php echo $iplist1[58]; ?>' >W05
            <input id="chk30" type="checkbox" name="ant[]" value='<?php echo $iplist1[60]; ?>' >W06 </font><br><hr>
            <input id="chk31" type="checkbox" name="ant[]" value='<?php echo $iplist1[62]; ?>' >Test1
            <input id="chk31" type="checkbox" name="ant[]" value='<?php echo $iplist1[64]; ?>' >Test2
           </div>
           </td>
           <td>
           <input type="image" src="connect.png" alt="Submit" width="150" height="50" style="margin-left:35px">
           </td>
		   </form>

        </tr>
      </table>
   </div>
</div>

<p>
<div class="inrect">
<div class="title"> <b>  Firmware Status  </b> </div>
 <div class="out">
  <table style="width:90%">

<?php
if((!empty($_POST['ant'])) || (isset($_POST['verify'])) || $_SESSION["installed"]==1)
{
     if(!empty($_POST['ant']))
     {
	 for($i=0;$i<30;$i++)
     {
	 $_SESSION["ip1"][$i]="";
	 $_SESSION["f1"][$i]="   ";
	 $_SESSION["d1"][$i]="   ";
	 $_SESSION["t1"][$i]="   ";
	 $_SESSION["f2"][$i]="   ";
	 $_SESSION["d2"][$i]="   ";
	 $_SESSION["t2"][$i]="   ";
	 $_SESSION["ant1"][$i]="   ";
	 $_SESSION["inst"][$i]="";
	 $_SESSION["installed"]=0;
	 $_SESSION["error"][$i]="";
	 $_SESSION["url"][$i] = "   ";
     }
	 $i=0;
     foreach($_POST['sys'] as $check1)
     {
     $_SESSION["system"]=$check1;
     }

	 foreach($_POST['ant'] as $check)
	 {
	 if(!empty($check))
	 {
	 for($j=0; $check[$j]!=" "; $j++)
	 $_SESSION["url"][$i][$j]= $check[$j];
	 $_SESSION["url"][$i].= "/up_Sum.xml";
	 $_SESSION["ant1"][$i]=$check[16].$check[17].$check[18];
	 $i++;
	 }
	 }
     }

    for($i=0;$i<30;$i++)
	{
      if($_SESSION["url"][$i][0]!=" ")
	  {
         $ch = curl_init();
         curl_setopt($ch, CURLOPT_URL, $_SESSION["url"][$i]);
         curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
		 curl_setopt($ch, CURLOPT_CONNECTTIMEOUT ,2);
		 curl_setopt($ch, CURLOPT_TIMEOUT, 2);
         $output = curl_exec($ch);
	     curl_close($ch);
         if($output)
         {
             $parser = simplexml_load_string($output);
             foreach($parser->P1 as $P1)
             {
			 $_SESSION["ip1"][$i]="{$P1}";
             }
             foreach($parser->F1 as $F1)
             {
			 $_SESSION["f1"][$i]="{$F1}";
             }
			 foreach($parser->F2 as $F2)
             {
			 $_SESSION["d1"][$i]="{$F2}";
             }
			 foreach($parser->F3 as $F3)
             {
			 $_SESSION["t1"][$i]="{$F3}";
             }
             foreach($parser->F4 as $F4)
             {
			 $_SESSION["f2"][$i]="{$F4}";
             }
             foreach($parser->F5 as $F5)
             {
			 $_SESSION["d2"][$i]="{$F5}";
             }
			 foreach($parser->F6 as $F6)
             {
			 $_SESSION["t2"][$i]="{$F6}";
             }
			 foreach($parser->F7 as $F7)
             {
			 $_SESSION["inst"][$i]="{$F7}";
             }
			 foreach($parser->E1 as $E1)
             {
			 $_SESSION["error"][$i]="{$E1}";
             }
		 }
         else if(empty($_POST['verify']) && $_SESSION["installed"]!=1)
         {
		 for($j=0;$_SESSION["url"][$i][$j]!='/';$j++)
		 $_SESSION["ip1"][$i]=$_SESSION["ip1"][$i].$_SESSION["url"][$i][$j];
         }
	  }
	}
}

if(!empty( $_SESSION['ip1'][0]))
{
echo ('<div><pre><h2 align="center"><font color="White" size="5" style="background:#00BFFF"><b>');
echo (" SYSTEM: ");
echo $_SESSION["system"];
echo (' </b></h2></pre></div>');
echo ('<tr><td colspan="5"><hr></tr><tr><pre><td align="left"><b>Ant</b></td><td align="center"><b>IP</td></b><td align="center"><b>Current Firmware</b></td>
      <td align="center"><b>Last Downloaded</b></td><td>
	  <form id="verify" action="" method="post">
      <input type="submit" style="margin-bottom:-18px" name="verify" value="Status">
      </form>
	  </td></pre></tr><tr><td colspan="5"><hr></tr>');
for($i=0;$i<30;$i++)
{
	if($_SESSION["url"][$i][0]!=" ")
	{
	if($_SESSION["f1"][$i]=="   ")
	echo "<tr><td><pre><font color='red' size='4'><b>";
    else
	echo "<tr><td><pre><font color='green' size='4'><b>";
	echo $_SESSION["ant1"][$i];
	echo "  </b></pre></td>";
    if($_SESSION["f1"][$i]=="   ")
	echo "<td><pre><font color='red' size='4'><b>  ";
    else
	echo "<td><pre><font color='green' size='4'><b>  ";
    echo $_SESSION["ip1"][$i];
	echo "  </b></pre></td>";
    if($_SESSION["f1"][$i]=="   ")
    {
	echo "<td align='center'><pre><font color='red' size='4'><b>Not available</b></font></pre></td>";
	}
    else
    {
	echo "<td><pre><font color='blue' size='4'><b>  ";
    echo $_SESSION["f1"][$i];
    echo "</b></font>";
	echo "<font size='3'> (";
	echo $_SESSION["d1"][$i];
	echo " ";
	echo $_SESSION["t1"][$i];
	echo ")  </font></pre></td>";
	}

	if($_SESSION["f2"][$i]=="   ")
    {
	echo "<td align='center'><pre><font color='red' size='4'><b>Not available</b></font></pre></td>";
	}
	else if($_SESSION["error"][$i]!="")
	{
	echo "<td align='center'><pre><font color='red' size='4'><b>";
	echo $_SESSION["error"][$i];
	echo "</b></font></pre></td>";
	}
	else
	{
	echo "<td><pre><font color='blue' size='4'><b>  ";
    echo $_SESSION["f2"][$i];
    echo "</b></font>";
	echo "<font size='3'> (";
	echo $_SESSION["d2"][$i];
	echo " ";
	echo $_SESSION["t2"][$i];
	echo ")</font></pre></td>";
	}

	if($_SESSION["installed"]==1 || isset($_POST['verify']))
	{
	if($_SESSION["inst"][$i]=="0" && $_SESSION["f1"][$i]!="   ")
	echo ('<td><pre>   <img src="check.png" style="width:30px;height:20px"></pre>');
    else
	echo ('<td><pre>   <img src="cross.png" style="width:20px;height:20px"></pre>');
	}
	}

echo ('<script> window.scrollTo(0,document.body.scrollHeight); </script>');
}
}
if(empty($_POST['ant']) && !empty($_POST['sys']))
{
echo "<script> alert('Select at least one Antenna'); </script>";
}
if(empty($_POST['ant']) && empty( $_SESSION['ip1'][0]))
{
echo "<b>Select at least one Antenna</b>";
}
?>
        </td>
      </tr>
    </table>
  </div>
</div>

<p>
<div class="inrect">
<div class="title"> <b>  Upload New Firmware  </b> </div>
 <div class="out">
  <table style="width:90%">
    <tr>
      <td align="center"><br>
         <form action="" method="post" enctype="multipart/form-data" name="form1" id="form1">
         <input type="file" name="firmware" id="firmware" >
         <input type="submit" name="upload" id="upload" value="Upload" >
         </form>
      </td>
    </tr>
  </table>
 </div>
</div>

<p>
<div class="inrect">
 <div class="title"> <?php if(empty($_POST['install'])) echo'<b>  Upload Status  </b>'; else echo'<b>  Update Status  </b>'; ?></div>
 <div class="out">
  <table style="width:100%">
  <tr>

<?php
if (isset($_POST['upload']))
{
// $upload_directory=dirname(__FILE__).'/fw/';
$upload_directory=dirname(c:/wamp64/www/fw).'/fw/';
$name="firmware.bin";
    if (!empty($_FILES['firmware']) && $_SESSION['login1'] == true)
	{
		$acceptedFormats = array('bin');
        if(!in_array(pathinfo($_FILES['firmware']['name'], PATHINFO_EXTENSION), $acceptedFormats))
		{
        echo '<font color="red" size="4"><b>  Wrong format (Select only .bin file) !!!  </b></font> <br><br>';
        }
		else
		{
    		if ($_FILES['firmware']['error'] > 0)
			{
            echo "Error: " . $_FILES["firmware"]["error"] ;
            }
			else
			{
			move_uploaded_file($_FILES['firmware']['tmp_name'],
			$upload_directory . $name);
			echo '<font color="green" size="4"><b>  Firmware Uploaded Successfully  </b></font> <br><br>';
			echo "Name: ";
			$_SESSION["upfile"] = $_FILES["firmware"]["name"];
			echo $_SESSION["upfile"];
			echo '<br><br>';
			echo "Size: ";
			$_SESSION["upsize"]=($_FILES['firmware']['size']);
			echo $_SESSION["upsize"];
			echo " bytes";
			echo('<form action="" method="POST">');
	        echo('<br><br><input type="submit" name="install" value="Install">');
            echo('</form>');
			echo ('<script> window.scrollTo(0,document.body.scrollHeight); </script>');
           }
		}
    }
	else
	{
	echo '<font color="red" size="4"><b>  Failed to upload  </b></font> <br><br>';
    }
}
else if (isset($_POST['install']) && !empty($_SESSION["f1"]))
{
echo '<font color="blue" size="4"><b>  Installing new Firmware  </b></font> <br><br>';
echo "Name: ";
echo $_SESSION["upfile"];
echo "<br>Size: ";
echo $_SESSION["upsize"];
echo " bytes<br><br>";
}
else
{
echo '<font color="blue" size="4"><b>  Ready to Upload  </b></font> <br><br>';
}
?>

<?php
if (isset($_POST['install']))
{
if(!empty($_SESSION["f1"]))
{
if ($_SESSION["url"][0][0]!=" ")
{
for($i=0;$i<30;$i++)
{
	if($_SESSION["url"][$i][0]!=" ")
	{
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $_SESSION["url"][$i]);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT_MS, 50);
    curl_setopt($ch, CURLOPT_TIMEOUT_MS, 50); //timeout in miliseconds
	curl_setopt($ch, CURLOPT_POSTFIELDS, 'FIRMWARE_URL=http://192.168.8.253/fw/firmware.bin');
	curl_exec($ch);
	curl_close($ch);
    }
}
echo('<div id="progress" align="left" style="width:250px; border-radius: 15px; border:4px double black; background:gray"></div>
<div id="information" style="width"></div>');
$total = 100;
for($i=0; $i<=$total; $i++)
{
    $percent = intval($i/$total * 100)."%";
    echo '<script language="javascript">
    document.getElementById("progress").innerHTML="<div style=\"width:'.$percent.'; border-radius: 15px; background-color:green;\">&nbsp;</div>";
    document.getElementById("information").innerHTML="'.$i.' % completed.";
    </script>';
    echo str_repeat(' ',1024*64);
    flush();
	time_nanosleep(0, 600000000);
	if($i==0)
	echo ('<script> window.scrollTo(0,document.body.scrollHeight); </script>');
}
sleep(1);
echo '<script> alert("******** Firmware update completed!! ********\n\nSelected Devices will reboot in few seconds."); </script>';
$_SESSION["installed"]=1;
sleep(1);
echo '<script>document.getElementById("verify").submit(); </script>';
}
}
else
{
echo '<script> alert("Select at least one Antenna"); </script>';
}
}
?>

  </tr>
  </table>
 </div>
</div>

<div style="font-size:12px; color:#bbccdd"> Designed @ Telemetry Lab - GMRT<br> </div>

</div>
</center>
<br>
</body>
</html>