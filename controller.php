<?php
    $rtype=$_POST['rtype'];
    $uuid=$_POST['uuid'];
    $source_ip=$_SERVER['REMOTE_ADDR'];
    $db_name="trace10";
    // echo $rtype;
    // var_dump($rtype);
    switch ($rtype) {
        case '24':
            # code...
            $on_ip=$_POST['on_ip'];
            $on_port=$_POST['on_port'];
            $in_ip=$_POST['in_ip'];
            $in_port=$_POST['in_port'];
            $pid=$_POST['pid'];
            $ktid=$_POST['ktid'];
            $tid=$_POST['tid'];
            $unit_uuid=$_POST['unit_uuid'];
            $ltime=$_POST['ttime'];
            $ftype=$_POST['ftype'];
            $dtype=$_POST['dtype'];
            $length=$_POST['length'];
            $supposed_length=$_POST['supposed_length'];
            $socket_id=$_POST['socket'];
            echo recordMessageToDatabase($on_ip,$on_port,$in_ip,$in_port,$pid,$ktid,$tid,$uuid,$unit_uuid,$ltime,$ftype,$length,$supposed_length,$socket_id,$source_ip,$dtype);
            break;
        case '25':
            $socket_id=$_POST['socket'];
            echo markSocketOpen($socket_id);
            break;
        case '26':
            $socket_id=$_POST['socket'];
            echo markSocketClose($socket_id);
            break;
        case '27':
            $pid=$_POST['pid'];
            $tid=$_POST['tid'];
            $ktid=$_POST['ktid'];
            $p_tid=$_POST['ptid'];
            $pk_tid=$_POST['pktid'];
            $p_pid=$_POST['ppid'];
            $ltime=$_POST['ttime'];
            echo recordThreadToDatabase($pid,$ktid,$tid,$p_pid,$pk_tid,$p_tid,$ltime,$source_ip);
            break;
        case '28':
            $pid=$_POST['pid'];
            $ktid=$_POST['ktid'];
            $d_tid=$_POST['dtid'];
            $j_tid=$_POST['jtid'];
            $ltime=$_POST['ttime'];
            echo recordThreadDependency($pid,$ktid,$d_tid,$j_tid,$ltime);
            break;
        default:
            # code...
            break;
    }
    
    
    //mark this socket connection as open
    //considering that socketid may be reused by different connection, there should be a method to generate socket uuid
    function markSocketOpen($socket_id){
        return 0;
    }
    
    //mark this socket connection as close
    function markSocketClose($socket_id){
        return 0;
    }
    
    
    //record this message to the local database, add event parents
    //char* ip,int port,pid_t pid,pthread_t tid,char* uuid,long long time,char type,int length
    function recordMessageToDatabase($on_ip,$on_port,$in_ip,$in_port,$pid,$ktid,$tid,$uuid,$unit_uuid,$ltime,$ftype,$length,$supposed_length,$socket_id,$source_ip,$dtype){
        $con = mysqli_connect("localhost","root","test1234",$GLOBALS['db_name']);
        if (mysqli_connect_errno($con))
        {
            die('Could not connect: '.mysqli_connect_error());
        }
        mysqli_autocommit($con,FALSE);  
        // mysql_query("BEGIN");
        $query="insert into trace_info(on_ip,on_port,in_ip,in_port,data_id,data_unit_id,pid,ktid,tid,ftype,ltime,length,supposed_length,connect_id,source_ip,dtype) values('$on_ip',$on_port,'$in_ip',$in_port,'$uuid','$unit_uuid',$pid,$ktid,$tid,$ftype,$ltime,$length,$supposed_length,$socket_id,'$source_ip',$dtype)";
        // echo $query;
        $result=mysqli_query($con,$query);
        // mysql_query("COMMIT");
        mysqli_commit($con);
        if($result){
            mysqli_close($con);
            return 0;
        }else{
            mysqli_close($con);
            return 1;
        }
        
    }

    //record this message to the local database, add event parents
    //char* ip,int port,pid_t pid,pthread_t tid,char* uuid,long long time,char type,int length
    function recordThreadToDatabase($pid,$ktid,$tid,$ppid,$pktid,$ptid,$ltime,$source){
        $con = mysqli_connect("localhost","root","test1234",$GLOBALS['db_name']);
        if (mysqli_connect_errno($con))
        {
            die('Could not connect: '.mysqli_connect_error());
        }
        mysqli_autocommit($con,FALSE);  
        // mysql_query("BEGIN");
        $query="insert into threads(process_id,k_thread_id,thread_id,p_process_id,pk_thread_id,p_thread_id,ltime,source_ip) values($pid,$ktid,$tid,$ppid,$pktid,$ptid,$ltime,'$source')";
        // echo $query;
        $result=mysqli_query($con,$query);
        // mysql_query("COMMIT");
        mysqli_commit($con);
        if($result){
            mysqli_close($con);
            return 0;
        }else{
            mysqli_close($con);
            return 1;
        }
        
    }

    function recordThreadDependency($pid,$ktid,$d_tid,$j_tid,$ltime){
        $con = mysqli_connect("localhost","root","test1234",$GLOBALS['db_name']);
        if (mysqli_connect_errno($con))
        {
            die('Could not connect: '.mysqli_connect_error());
        }
        mysqli_autocommit($con,FALSE);  
        // mysql_query("BEGIN");
        $query="insert into thread_dep(process_id,k_thread_id,d_thread_id,j_thread_id,ltime,source_ip) values($pid,$ktid,$d_tid,$j_tid,$ltime,'$source')";
        // echo $query;
        $result=mysqli_query($con,$query);
        // mysql_query("COMMIT");
        mysqli_commit($con);
        if($result){
            mysqli_close($con);
            return 0;
        }else{
            mysqli_close($con);
            return 1;
        }
    }
    
    ?>