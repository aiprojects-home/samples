use store

drop table Orders
drop table Customers

create table Customers (Id INT NOT NULL, Name VARCHAR(MAX), PRIMARY KEY (Id))
create table Orders (Id INT NOT NULL, CustomerId INT NOT NULL,
PRIMARY KEY(Id), FOREIGN KEY (CustomerId) REFERENCES Customers(Id) )

insert into Customers values (1, 'Max')
insert into Customers values (2, 'Pavel')
insert into Customers values (3, 'Ivan')
insert into Customers values (4, 'Leonid')

select * from Customers

insert into Orders values (1, 2)
insert into Orders values (2, 4)

select * from Orders

select Customers.Name as Customers from Customers where Customers.Id
not in (select Orders.CustomerId from Orders)