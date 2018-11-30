begin;
	drop table if exists info_show;
commit;
begin;
	create table if not exists info_show(info_id int(64) not null, info varchar(255));
commit;
begin;
	insert into info_show values( 1, '---------begin drop tables----------------------------');
	insert into info_show values( 2, '---------end drop tables------------------------------');	
	insert into info_show values( 3, '---------begin create tables--------------------------');
	insert into info_show values( 4, '---------end create tables----------------------------');
	insert into info_show values( 5, '---------begin insert data 1 2 3 4 5 6 7--------------');
	insert into info_show values( 6, '---------end insert data 1 2 3 4 5 6 7----------------');
	insert into info_show values( 7, '---------begin delete data 6--------------------------');
	insert into info_show values( 8, '---------end delete data 6----------------------------');
	insert into info_show values( 9, '---------begin update data 5--------------------------');
	insert into info_show values(10, '---------end update data 5----------------------------');
	insert into info_show values(11, '---------begin delete all data------------------------');
	insert into info_show values(12, '---------end delete all data--------------------------');
	insert into info_show values(13, '---------begin drop table-----------------------------');
	insert into info_show values(14, '---------end drop table-------------------------------');
commit;
select info from info_show where info_id = 1;
begin;
	drop table if exists t_file_auth;
commit;
select info from info_show where info_id = 2;
select info from info_show where info_id = 3;
begin;
	create table if not exists t_file_auth(file_id int(64) not null, dev_id int(64), file_name varchar(255), file_type int(16), file_role int (32), file_lev int(8),new_line varchar(2));
commit;
select info from info_show where info_id = 4;
select info from info_show where info_id = 5;
begin;
	insert into t_file_auth values(1, 1, 'dzb', 1, 1, 1,'\n');
	insert into t_file_auth values(2, 2, 'aaa', 2, 2, 2,'\n');
	insert into t_file_auth values(3, 3, 'bbb', 3, 3, 3,'\n');
	insert into t_file_auth values(4, 4, 'ccc', 4, 4, 4,'\n');
	insert into t_file_auth values(5, 5, 'ddd', 5, 5, 5,'\n');
	insert into t_file_auth values(6, 6, 'eee', 6, 6, 6,'\n');
	insert into t_file_auth values(7, 7, 'fff', 7, 7, 7,'\n');
	--if something wrong use rollback;
commit;
select info from info_show where info_id = 6;
select * from t_file_auth;
select info from info_show where info_id = 7;
begin;
	delete from t_file_auth where file_id = 6;
commit;
select info from info_show where info_id = 8;
select * from t_file_auth;
select info from info_show where info_id = 9;
begin;
	update t_file_auth set file_id = 9,dev_id = 9,file_name = 'ggg',file_type = 9,file_role = 9, file_lev = 9 where file_id = 5;
commit;
select info from info_show where info_id = 10;
select * from t_file_auth;
--select info from info_show where info_id = 11;
--begin;
--	delete from t_file_auth;
--commit;
--select info from info_show where info_id = 12;
--select * from t_file_auth;
--select info from info_show where info_id = 13;
--begin;
--	drop table if exists t_file_auth;
--commit;
--select info from info_show where info_id = 14;
