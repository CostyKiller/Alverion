package org.l2jmobius.gameserver.data.sql;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.l2jmobius.commons.database.DatabaseFactory;
import org.l2jmobius.commons.network.buffer.WriteBuffer;

/**
 * Alverion appearance extension.<br>
 * Keeps each character's appearance block exactly as the Alverion client sent it at creation, and writes it back to that client.<br>
 * The server never reads the bytes; only the client gives them meaning.<br>
 * On the wire the block follows a magic number, so a client that never sends one and a server without this patch both carry on as before.
 */
public class CharacterAppearanceTable
{
	private static final Logger LOGGER = Logger.getLogger(CharacterAppearanceTable.class.getName());
	
	/** The bytes 'A' 'L' 'V' '1', read as a little-endian int. */
	public static final int MAGIC = 0x31564C41;
	
	/** The longest block accepted. The client sends 24 bytes today. */
	public static final int MAX_SIZE = 64;
	
	private static final byte[] NONE = new byte[0];
	
	private final Map<Integer, byte[]> _appearances = new ConcurrentHashMap<>();
	
	protected CharacterAppearanceTable()
	{
	}
	
	/**
	 * @param objectId the character's object id
	 * @return the stored block, or an empty array for a character created without one
	 */
	public byte[] get(int objectId)
	{
		return _appearances.computeIfAbsent(objectId, this::load);
	}
	
	public void store(int objectId, byte[] appearance)
	{
		if ((appearance == null) || (appearance.length == 0) || (appearance.length > MAX_SIZE))
		{
			return;
		}
		
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement ps = con.prepareStatement("REPLACE INTO character_appearance (charId, data) VALUES (?, ?)"))
		{
			ps.setInt(1, objectId);
			ps.setBytes(2, appearance);
			ps.execute();
			_appearances.put(objectId, appearance);
		}
		catch (Exception e)
		{
			LOGGER.log(Level.WARNING, getClass().getSimpleName() + ": Could not store the appearance of " + objectId + ".", e);
		}
	}
	
	public void delete(int objectId)
	{
		_appearances.remove(objectId);
		
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement ps = con.prepareStatement("DELETE FROM character_appearance WHERE charId=?"))
		{
			ps.setInt(1, objectId);
			ps.execute();
		}
		catch (Exception e)
		{
			LOGGER.log(Level.WARNING, getClass().getSimpleName() + ": Could not delete the appearance of " + objectId + ".", e);
		}
	}
	
	private byte[] load(int objectId)
	{
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement ps = con.prepareStatement("SELECT data FROM character_appearance WHERE charId=?"))
		{
			ps.setInt(1, objectId);
			try (ResultSet rs = ps.executeQuery())
			{
				if (rs.next())
				{
					final byte[] data = rs.getBytes(1);
					return data != null ? data : NONE;
				}
			}
		}
		catch (Exception e)
		{
			LOGGER.log(Level.WARNING, getClass().getSimpleName() + ": Could not load the appearance of " + objectId + ".", e);
		}
		
		return NONE;
	}
	
	/**
	 * Appends the blocks to the end of a packet: every block padded to the longest, then that size and the count as shorts, then the magic.<br>
	 * The client reads it from the end, so nothing before it changes. Writes nothing when no block exists, leaving the packet exactly as stock.
	 * @param buffer the packet being written
	 * @param blocks one block per entry the packet lists, in order, empty for an entry without one
	 */
	public static void writeTrailer(WriteBuffer buffer, List<byte[]> blocks)
	{
		int size = 0;
		for (byte[] block : blocks)
		{
			size = Math.max(size, block.length);
		}
		
		if (size == 0)
		{
			return;
		}
		
		for (byte[] block : blocks)
		{
			buffer.writeBytes(block);
			for (int i = block.length; i < size; i++)
			{
				buffer.writeByte(0);
			}
		}
		
		buffer.writeShort(size);
		buffer.writeShort(blocks.size());
		buffer.writeInt(MAGIC);
	}
	
	public static CharacterAppearanceTable getInstance()
	{
		return SingletonHolder.INSTANCE;
	}
	
	private static class SingletonHolder
	{
		protected static final CharacterAppearanceTable INSTANCE = new CharacterAppearanceTable();
	}
}
