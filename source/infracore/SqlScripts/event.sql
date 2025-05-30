CREATE EVENT update_status
ON SCHEDULE EVERY 1000 SECOND
DO
UPDATE active progress 
SET progress.active = 0 
WHERE TIMESTAMPDIFF(HOUR,now(),progress.tstamp)
>3

;


/*INSERT INTO Persons (FirstName) VALUES ('Joe');
SELECT ID AS LastID FROM Persons WHERE ID = @@Identity;
*/
