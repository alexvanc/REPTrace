drop table if exists `trace_info`;
create table `trace_info`(
	`id` int not null primary key auto_increment,
	`on_ip` text not null,
	`on_port` int not null,
	`in_ip` text not null,
	`in_port` int not null,
	`data_id` text not null,
	`data_unit_id` text not null,
	`connect_id` int,
	`connect_uuid` varchar(255),
	`pid` int not null,
	`ktid` int not null,
	`tid` bigint not null,
	`ftype` tinyint not null,
	`dtype` tinyint not null,
	`time` timestamp default CURRENT_TIMESTAMP,
	`ltime` bigint(20) not null,
	`supposed_length` bigint(20) not null,
	`length` bigint(20) not null,
	`source_ip` text
);

drop table if exists `request_type`;
create table `request_type`(
	`id` int not null primary key auto_increment,
	`number` int not null,
	`description` varchar(255)
);
 
insert into `request_type`(number,description) values (20,'check_send');
insert into `request_type`(number,description) values (21,'mark_send');
insert into `request_type`(number,description) values (22,'check_recv');
insert into `request_type`(number,description) values (23,'mark_recv');
insert into `request_type`(number,description) values (24,'push_database');
insert into `request_type`(number,description) values (25,'open_socket');
insert into `request_type`(number,description) values (26,'close_socket');


drop table if exists `func_type`;
create table `func_type`(
	`id` int not null primary key auto_increment,
	`number` int not null,
	`description` varchar(255)
);
insert into `func_type`(number,description) values (1,'send'); 
insert into `func_type`(number,description) values (2,'recv');
insert into `func_type`(number,description) values (3,'write');
insert into `func_type`(number,description) values (4,'read');
insert into `func_type`(number,description) values (5,'sendmsg');
insert into `func_type`(number,description) values (6,'recvmsg');
insert into `func_type`(number,description) values (7,'sendto');
insert into `func_type`(number,description) values (8,'recvfrom');
insert into `func_type`(number,description) values (9,'writev');
insert into `func_type`(number,description) values (10,'readv');
insert into `func_type`(number,description) values (11,'sendmmsg');
insert into `func_type`(number,description) values (12,'recvmmsg');
insert into `func_type`(number,description) values (13,'connect');
insert into `func_type`(number,description) values (14,'socket');
insert into `func_type`(number,description) values (15,'close');



drop table if exists `finish_type`;
create table `finish_type`(
	`id` int not null primary key auto_increment,
	`number` int not null,
	`description` varchar(255)
);
insert into `finish_type`(number,description) values (1,'send done normally');
insert into `finish_type`(number,description) values (2,'send done error');
insert into `finish_type`(number,description) values (3,'send done with uuid');
insert into `finish_type`(number,description) values (4,'send done with filter');
insert into `finish_type`(number,description) values (5,'recv done without uuid');
insert into `finish_type`(number,description) values (6,'recv done with filter');
insert into `finish_type`(number,description) values (7,'recv done with error');
insert into `finish_type`(number,description) values (8,'recv done with single uuid');
insert into `finish_type`(number,description) values (9,'recv done with multiple uuids');
insert into `finish_type`(number,description) values (10,'recv done with single and broken uuid');
insert into `finish_type`(number,description) values (11,'recv done with multiple and broken uuids');
insert into `finish_type`(number,description) values (12,'done with IPv6');
insert into `finish_type`(number,description) values (13,'done with UNIX socket');
insert into `finish_type`(number,description) values (14,'done with other sockets');
insert into `finish_type`(number,description) values (15,'recv done with 2 and ID_LENGTH');
insert into `finish_type`(number,description) values (16,'recv done with 2 and ID_LENGTH and real uuid');
insert into `finish_type`(number,description) values (17,'recv done with 2 not ID_LENGTH');
insert into `finish_type`(number,description) values (18,'recv done with 1');
insert into `finish_type`(number,description) values (19,'recv done with 1 and real uuid');
insert into `finish_type`(number,description) values (20,'recv done normally and real uuid');

drop table if exists `thread_dep`;
create table `thread_dep`(
	`id` int not null primary key auto_increment,
	`process_id` int not null,
	`k_thread_id` int not null,
	`d_thread_id` bigint not null,
	`j_thread_id` bigint not null,
	`time` timestamp default CURRENT_TIMESTAMP,
	`ltime` bigint(20) not null,
	`source_ip` text
);

drop table if exists `threads`;
create table `threads`(
	`id` int not null primary key auto_increment,
	`process_id` int not null,
	`k_thread_id` int not null,
	`thread_id` bigint not null,
	`p_thread_id` bigint,
	`pk_thread_id` int not null,
	`p_process_id` int default 0,
	`time` timestamp default CURRENT_TIMESTAMP,
	`ltime` bigint(20) not null,
	`deleted` int default 0,
	`source_ip` text,
	`t_family` int,
	`t_parent` int,
	`ind_t_family` int,
	`ind_t_parent` int,
	`ind_parent` int

);

